/*
 * This file is protected by Copyright. Please refer to the COPYRIGHT file
 * distributed with this source distribution.
 *
 * This file is part of OpenCPI <http://www.opencpi.org>
 *
 * OpenCPI is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _OCPI_PROJECTS_LOG_FORWARDER_HH
#define _OCPI_PROJECTS_LOG_FORWARDER_HH

#include <cstdarg> // va_list

namespace OCPIProjects {

typedef void (*OCPI_log_func_args_t)(unsigned, const char*, va_list);

/*! @brief Helper class which provides a generic logging API as well as a
 *         registration mechanism which causes log messages to be forwarded to
 *         a specific vprintf-esque logging API.
 ******************************************************************************/
template<class T>
class LogForwarder {
}; // class LogForwarder

/*! @brief Helper class which provides a generic logging API as well as a
 *         registration mechanism which causes log messages to be forwarded to
 *         a specific custom vprintf-esque logging API.
 *         This class can use vprintf directly:
 * - CODE:
   @verbatim
    LogForwarder<int (*)(const char*, va_list)> forwarder;
    forwarder.set_log_forwarding_callback(vprintf);
    forwarder.log_info("TEST LOG %i\n", 5);
   @endverbatim
 ******************************************************************************/
template<>
class LogForwarder<int (*)(const char*, va_list)> {

protected : int (*m_callback_log_info) (const char*, va_list);
protected : int (*m_callback_log_debug)(const char*, va_list);
protected : int (*m_callback_log_trace)(const char*, va_list);
protected : int (*m_callback_log_warn) (const char*, va_list);
protected : int (*m_callback_log_error)(const char*, va_list);

public    : LogForwarder();

public    : void log_info(const char* msg, va_list arg) const;
public    : void log_debug(const char* msg, va_list arg) const;
public    : void log_trace(const char* msg, va_list arg) const;
public    : void log_warn(const char* msg, va_list arg) const;
public    : void log_error(const char* msg, va_list arg) const;

public    :
#ifdef __GNUC__
__attribute__((format(printf, 2, 3)))
#endif
void log_info(const char* msg, ...) const;

public    :
#ifdef __GNUC__
__attribute__((format(printf, 2, 3)))
#endif
void log_debug(const char* msg, ...) const;

public    :
#ifdef __GNUC__
__attribute__((format(printf, 2, 3)))
#endif
void log_trace(const char* msg, ...) const;

public    :
#ifdef __GNUC__
__attribute__((format(printf, 2, 3)))
#endif
void log_warn(const char* msg, ...) const;

public    :
#ifdef __GNUC__
__attribute__((format(printf, 2, 3)))
#endif
void log_error(const char* msg, ...) const;

public    : void set_forwarding_callback_log_info(
                int (*callback_func_ptr)(const char*, va_list arg));
public    : void set_forwarding_callback_log_debug(
                int (*callback_func_ptr)(const char*, va_list arg));
public    : void set_forwarding_callback_log_trace(
                int (*callback_func_ptr)(const char*, va_list arg));
public    : void set_forwarding_callback_log_warn(
                int (*callback_func_ptr)(const char*, va_list arg));
public    : void set_forwarding_callback_log_error(
                int (*callback_func_ptr)(const char*, va_list arg));
public    : void clear_forwarding_callback_log_info();
public    : void clear_forwarding_callback_log_debug();
public    : void clear_forwarding_callback_log_trace();
public    : void clear_forwarding_callback_log_warn();
public    : void clear_forwarding_callback_log_error();

}; // class LogForwarder<int (*)(const char*, va_list)>

/*! @brief Helper class which provides a generic logging API as well as a
 *         registration mechanism which causes log messages to be forwarded to
 *         custom logging API.
 *         Example use with the OpenCPI logging API:
 *         LogForwarder<void (*)(unsigned, const char*, va_list)> forwarder;
 *         forwarder.set_forwarding_callback_log_info(OCPI::OS::logPrintV);
 *         forwarder.set_arg0_log_info(OCPI_LOG_INFO);
 *         forwarder.set_forwarding_callback_log_debug(OCPI::OS::logPrintV);
 *         forwarder.set_arg0_log_debug(OCPI_LOG_DEBUG);
 *         forwarder.log_info("************** TEST LOG TEST LOG %i\n", 5);
 *         forwarder.log_debug("************** TEST LOG TEST LOG %i\n", 55);
 ******************************************************************************/
template<>
class LogForwarder <void (*)(unsigned, const char*, va_list)> {

protected : void (*m_callback_log_info) (unsigned, const char*, va_list);
protected : void (*m_callback_log_debug)(unsigned, const char*, va_list);
protected : void (*m_callback_log_trace)(unsigned, const char*, va_list);
protected : void (*m_callback_log_warn) (unsigned, const char*, va_list);
protected : void (*m_callback_log_error)(unsigned, const char*, va_list);

protected : unsigned m_arg0_log_info;
protected : unsigned m_arg0_log_debug;
protected : unsigned m_arg0_log_trace;
protected : unsigned m_arg0_log_warn;
protected : unsigned m_arg0_log_error;

public    : LogForwarder();

public    : void log_info(const char* msg, va_list arg) const;
public    : void log_debug(const char* msg, va_list arg) const;
public    : void log_trace(const char* msg, va_list arg) const;
public    : void log_warn(const char* msg, va_list arg) const;
public    : void log_error(const char* msg, va_list arg) const;

public    :
#ifdef __GNUC__
__attribute__((format(printf, 2, 3)))
#endif
void log_info(const char* msg, ...) const;

public    :
#ifdef __GNUC__
__attribute__((format(printf, 2, 3)))
#endif
void log_debug(const char* msg, ...) const;

public    :
#ifdef __GNUC__
__attribute__((format(printf, 2, 3)))
#endif
void log_trace(const char* msg, ...) const;

public    :
#ifdef __GNUC__
__attribute__((format(printf, 2, 3)))
#endif
void log_warn(const char* msg, ...) const;

public    :
#ifdef __GNUC__
__attribute__((format(printf, 2, 3)))
#endif
void log_error(const char* msg, ...) const;

public    : void set_forwarding_callback_log_info(
                void (*callback_func_ptr)(unsigned, const char*, va_list arg));
public    : void set_forwarding_callback_log_debug(
                void (*callback_func_ptr)(unsigned, const char*, va_list arg));
public    : void set_forwarding_callback_log_trace(
                void (*callback_func_ptr)(unsigned, const char*, va_list arg));
public    : void set_forwarding_callback_log_warn(
                void (*callback_func_ptr)(unsigned, const char*, va_list arg));
public    : void set_forwarding_callback_log_error(
                void (*callback_func_ptr)(unsigned, const char*, va_list arg));

public    : void clear_forwarding_callback_log_info();
public    : void clear_forwarding_callback_log_debug();
public    : void clear_forwarding_callback_log_trace();
public    : void clear_forwarding_callback_log_warn();
public    : void clear_forwarding_callback_log_error();

public    : void set_arg0_log_info(unsigned arg0);
public    : void set_arg0_log_debug(unsigned arg0);
public    : void set_arg0_log_trace(unsigned arg0);
public    : void set_arg0_log_warn(unsigned arg0);
public    : void set_arg0_log_error(unsigned arg0);

}; // class LogForwarder <void (*)(unsigned, const char*, va_list)>

} // namespace OCPIProjects

#include "LogForwarder.cc"

#endif // _OCPI_PROJECTS_LOG_FORWARDER_HH
