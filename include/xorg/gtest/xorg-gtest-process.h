/*******************************************************************************
 *
 * X testing environment - Google Test environment feat. dummy x server
 *
 * Copyright (C) 2011, 2012 Canonical Ltd.
 * Copyright © 2012 Red Hat, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 ******************************************************************************/

#ifndef XORG_GTEST_PROCESS_H
#define XORG_GTEST_PROCESS_H

#include <stdarg.h>
#include <X11/Xfuncproto.h> /* for _X_SENTINEL */

#include <memory>
#include <string>

namespace xorg {
namespace testing {

/**
 * @class Process xorg-gtest-process.h xorg/gtest/xorg-gtest-process.h
 *
 * Class that abstracts child process creation and termination.
 *
 * This class allows for forking, running and terminating child processes.
 * In addition, manipulation of the child process' environment is supported.
 * For example, starting an X server instance on display port 133 as a child
 * process can be realized with the following code snippet:
 * @code
 * Process xorgServer;
 * try {
 *   xorgServer.Start("Xorg", "Xorg", ":133");
 * } catch (const std::runtime_error&e) {
 *   std::cerr << "Problem starting the X server: " << e.what() << std::endl;
 * }
 * ...
 * if (!xorgServer.Terminate()) {
 *   std::cerr << "Problem terminating server ... killing now ..." << std::endl;
 *   if (!xorgServer.Kill())
 *     std::cerr << "Problem killing server" << std::endl;
 * }
 * @endcode
 */
class Process {
 public:
   /**
    * Describes the state of a process as seen by this library. This state
    * changes some behaviors inside the library, most notably:
    * * A process in state ERROR or NONE will fail to Kill() or Terminate()
    * * A process in state FINISHED_SUCCESS or FINISHED_FAILURE will always
    * succeed to Kill() or Terminate()
    * * A process in state TERMINATED may change state to FINISHED_SUCCESS
    * or FINISHED_FAILURE when queried again.
    */
   enum State {
     ERROR,             /**< An error has occured, state is now unknown */
     NONE,              /**< The process has not been started yet */
     RUNNING,           /**< The process has been started */
     FINISHED_SUCCESS,  /**< The process finished with an exit code of 0 */
     FINISHED_FAILURE,  /**< The process finished with a non-zero exit code */
     TERMINATED,        /**< The process was successfully terminated by this
                             library but it's state is currently unknown */
   };

  /**
   * Helper function to adjust the environment of the current process.
   *
   * @param [in] name Name of the environment variable.
   * @param [in] value Value of the environment variable.
   * @param [in] overwrite Whether to overwrite the value of existing env
   *             variables.
   *
   * @throws std::runtime_error if adjusting the environment does not succeed.
   */
  static void SetEnv(const std::string& name, const std::string& value,
                     bool overwrite);

  /**
   * Helper function to query the environment of the current process.
   *
   * @param [in] name The name of the environment variable.
   * @param [out] exists If not NULL, the variable will be set to true if the
   *              environment variable exists and to false otherwise.
   * @returns The value of the environment variable, or an empty string.
   */
  static std::string GetEnv(const std::string& name, bool* exists = NULL);

  /**
   * Creates a child-process that is in a terminated state.
   */
  Process();

  /**
   * Fork manually. Usually, fork() is called as part of Start() but for
   * use-cases where the parent process and the child process need special
   * processing before the child is replaced by an execvp call Fork() may be
   * called manually.
   *
   * A process may only be forked once.
   *
   * The state of both the parent and the child after a Fork() is
   * Process::RUNNING. If Fork() is called directly, Start() may only be
   * called on the child process.
   *
   * @throws std::runtime_error on failure.
   *
   * @return The pid of the child, or 0 for the child process.
   */
  pid_t Fork();

  /**
   * Starts a program as a child process.
   *
   * See 'man execvp' for further information on the elements in
   * the vector.
   *
   * If Fork() was called previously, Start() may only be called on the child
   * process.
   *
   * @param program The program to start.
   * @param args Vector of arguments passed to the program.
   *
   * @throws std::runtime_error on failure.
   *
   * @post If successful: Child process forked and program started.
   * @post If successful: Subsequent calls to Pid() return child process pid.
   */
  void Start(const std::string& program, const std::vector<std::string> &args);

  /**
   * Starts a program as a child process.
   *
   * See 'man execvp' for further information on the variadic argument list.
   *
   * If Fork() was called previously, Start() may only be called on the child
   * process.
   *
   * @param program The program to start.
   * @param args Variadic list of arguments passed to the program. This list
   * must end with NULL.
   *
   * @throws std::runtime_error on failure.
   *
   * @post If successful: Child process forked and program started.
   * @post If successful: Subsequent calls to Pid() return child process pid.
   */
  void Start(const std::string& program, va_list args);

  /**
   * Starts a program as a child process.
   *
   * Takes a variadic list of arguments passed to the program. This list
   * must end with NULL.
   * See 'man execvp' for further information on the variadic argument list.
   *
   * If Fork() was called previously, Start() may only be called on the child
   * process.
   *
   * @param program The program to start.
   *
   * @throws std::runtime_error on failure.
   *
   * @post If successful: Child process forked and program started.
   * @post If successful: Subsequent calls to Pid() return child process pid.
   */
  void Start(const std::string& program, ...) _X_SENTINEL(0);

  /**
   * Terminates (SIGTERM) this child process and waits a given timeout for
   * the process to terminate.
   *
   * @param [in] timeout The timeout in millis to wait for the process to
   *                     terminate. A timeout of 0 implies not to wait but
   *                     return immediately.
   *
   * @throws std::runtime_error if child tries to terminate itself.
   *
   * @returns true if termination succeeded and, if a timout is given, the
   *          process shut down within that timeout. false otherwise.
   *
   * @post If successful: Child process terminated.
   * @post If successful: Subsequent calls to Pid() return -1.
   */
  virtual bool Terminate(unsigned int timeout = 0);

  /**
   * Kills (SIGKILL) this child process and waits a given timeout for the
   * process to terminate.
   *
   * @param [in] timeout The timeout in millis to wait for the process to
   *                     terminate. A timeout of 0 implies not to wait but
   *                     return immediately.
   *
   * @throws std::runtime_error if child tries to kill itself.
   *
   * @returns true if kill succeeded and, if a timout is given, the
   *          process shut down within that timeout. false otherwise.
   *
   * @post If successful: Child process killed.
   * @post If successful: Subsequent calls to Pid() return -1.
   */
  virtual bool Kill(unsigned int timeout = 0);

  /**
   * Accesses the pid of the child process.
   *
   * @returns The pid of the child process or -1.
   */
  pid_t Pid() const;

  /**
   * Return the state of the process.
   *
   * @return The current state of the process
   */
  enum Process::State GetState();

 private:
  struct Private;
  std::auto_ptr<Private> d_;

  /* Disable copy constructor, assignment operator */
  Process(const Process&);
  Process& operator=(const Process&);
  bool WaitForExit(unsigned int timeout);
  bool KillSelf(int signal, unsigned int timout);
};

} // testing
} // xorg

#endif // XORG_GTEST_PROCESS_H
