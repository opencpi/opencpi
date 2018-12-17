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

#include <cstdarg> // va_list, va_start(), va_end()
#include <cstdio>  // printf()
#include "LogForwarder.hh"

namespace OCPIProjects {

LogForwarder<int (*)(const char*, va_list)>::LogForwarder() {
  clear_forwarding_callback_log_info();
  clear_forwarding_callback_log_debug();
  clear_forwarding_callback_log_trace();
  clear_forwarding_callback_log_warn();
  clear_forwarding_callback_log_error();
}

void LogForwarder<int (*)(const char*, va_list)>::log_info(
    const char* msg, ...) const {

  va_list arg;
  va_start(arg, msg);
  log_info(msg, arg);
  va_end(arg);
}

void LogForwarder<int (*)(const char*, va_list)>::log_debug(
    const char* msg, ...) const {

  va_list arg;
  va_start(arg, msg);
  log_debug(msg, arg);
  va_end(arg);
}

void LogForwarder<int (*)(const char*, va_list)>::log_trace(
    const char* msg, ...) const {

  va_list arg;
  va_start(arg, msg);
  log_trace(msg, arg);
  va_end(arg);
}

void LogForwarder<int (*)(const char*, va_list)>::log_warn(
    const char* msg, ...) const {

  va_list arg;
  va_start(arg, msg);
  log_warn(msg, arg);
  va_end(arg);
}

void LogForwarder<int (*)(const char*, va_list)>::log_error(
    const char* msg, ...) const {

  va_list arg;
  va_start(arg, msg);
  log_error(msg, arg);
  va_end(arg);
}

void LogForwarder<int (*)(const char*, va_list)>::log_info(
    const char* msg, va_list arg) const {

  if(m_callback_log_info == 0) {
    return;
  }
  m_callback_log_info(msg, arg);
  printf("\n");
}

void LogForwarder<int (*)(const char*, va_list)>::log_debug(
    const char* msg, va_list arg) const {

  if(m_callback_log_debug == 0) {
    return;
  }
  m_callback_log_debug(msg, arg);
  printf("\n");
}

void LogForwarder<int (*)(const char*, va_list)>::log_trace(
    const char* msg, va_list arg) const {

  if(m_callback_log_trace == 0) {
    return;
  }
  m_callback_log_trace(msg, arg);
  printf("\n");
}

void LogForwarder<int (*)(const char*, va_list)>::log_warn(
    const char* msg, va_list arg) const {

  if(m_callback_log_warn == 0) {
    return;
  }
  m_callback_log_warn(msg, arg);
  printf("\n");
}

void LogForwarder<int (*)(const char*, va_list)>::log_error(
    const char* msg, va_list arg) const {

  if(m_callback_log_error == 0) {
    return;
  }
  m_callback_log_error(msg, arg);
  printf("\n");
}

void LogForwarder<int (*)(const char*, va_list)>::set_forwarding_callback_log_info(
    int (*callback_func_ptr)(const char*, va_list arg)) {
  m_callback_log_info = callback_func_ptr;
}

void LogForwarder<int (*)(const char*, va_list)>::set_forwarding_callback_log_debug(
    int (*callback_func_ptr)(const char*, va_list arg)) {
  m_callback_log_debug = callback_func_ptr;
}

void LogForwarder<int (*)(const char*, va_list)>::set_forwarding_callback_log_trace(
    int (*callback_func_ptr)(const char*, va_list arg)) {
  m_callback_log_trace = callback_func_ptr;
}

void LogForwarder<int (*)(const char*, va_list)>::set_forwarding_callback_log_warn(
    int (*callback_func_ptr)(const char*, va_list arg)) {
  m_callback_log_warn = callback_func_ptr;
}

void LogForwarder<int (*)(const char*, va_list)>::set_forwarding_callback_log_error(
    int (*callback_func_ptr)(const char*, va_list arg)) {
  m_callback_log_error = callback_func_ptr;
}

void LogForwarder<int (*)(const char*, va_list)>::clear_forwarding_callback_log_info() {
  m_callback_log_info = 0;
}

void LogForwarder<int (*)(const char*, va_list)>::clear_forwarding_callback_log_debug() {
  m_callback_log_debug = 0;
}

void LogForwarder<int (*)(const char*, va_list)>::clear_forwarding_callback_log_trace() {
  m_callback_log_trace = 0;
}

void LogForwarder<int (*)(const char*, va_list)>::clear_forwarding_callback_log_warn() {
  m_callback_log_warn = 0;
}

void LogForwarder<int (*)(const char*, va_list)>::clear_forwarding_callback_log_error() {
  m_callback_log_error = 0;
}

/// @brief 0 is the assumed to be "smart default" for arg0
LogForwarder<void (*)(unsigned, const char*, va_list)>::LogForwarder() {
  clear_forwarding_callback_log_info();
  clear_forwarding_callback_log_debug();
  clear_forwarding_callback_log_trace();
  clear_forwarding_callback_log_warn();
  clear_forwarding_callback_log_error();
  set_arg0_log_info(0);
  set_arg0_log_debug(0);
  set_arg0_log_trace(0);
  set_arg0_log_warn(0);
  set_arg0_log_error(0);
}

void LogForwarder<void (*)(unsigned, const char*, va_list)>::log_info(
    const char* msg, ...) const {

  va_list arg;
  va_start(arg, msg);
  log_info(msg, arg);
  va_end(arg);
}

void LogForwarder<void (*)(unsigned, const char*, va_list)>::log_debug(
    const char* msg, ...) const {

  va_list arg;
  va_start(arg, msg);
  log_debug(msg, arg);
  va_end(arg);
}

void LogForwarder<void (*)(unsigned, const char*, va_list)>::log_trace(
    const char* msg, ...) const {

  va_list arg;
  va_start(arg, msg);
  log_trace(msg, arg);
  va_end(arg);
}

void LogForwarder<void (*)(unsigned, const char*, va_list)>::log_warn(
    const char* msg, ...) const {

  va_list arg;
  va_start(arg, msg);
  log_warn(msg, arg);
  va_end(arg);
}

void LogForwarder<void (*)(unsigned, const char*, va_list)>::log_error(
    const char* msg, ...) const {

  va_list arg;
  va_start(arg, msg);
  log_error(msg, arg);
  va_end(arg);
}

void LogForwarder<void (*)(unsigned, const char*, va_list)>::log_info(
    const char* msg, va_list arg) const {

  if(m_callback_log_info == 0) {
    return;
  }
  m_callback_log_info(m_arg0_log_info, msg, arg);
}

void LogForwarder<void (*)(unsigned, const char*, va_list)>::log_debug(
    const char* msg, va_list arg) const {

  if(m_callback_log_debug == 0) {
    return;
  }
  m_callback_log_debug(m_arg0_log_debug, msg, arg);
}

void LogForwarder<void (*)(unsigned, const char*, va_list)>::log_trace(
    const char* msg, va_list arg) const {

  if(m_callback_log_trace == 0) {
    return;
  }
  m_callback_log_trace(m_arg0_log_trace, msg, arg);
}

void LogForwarder<void (*)(unsigned, const char*, va_list)>::log_warn(
    const char* msg, va_list arg) const {

  if(m_callback_log_warn == 0) {
    return;
  }
  m_callback_log_warn(m_arg0_log_warn, msg, arg);
}

void LogForwarder<void (*)(unsigned, const char*, va_list)>::log_error(
    const char* msg, va_list arg) const {

  if(m_callback_log_error == 0) {
    return;
  }
  m_callback_log_error(m_arg0_log_error, msg, arg);
}

void LogForwarder<void (*)(unsigned, const char*, va_list)>::set_forwarding_callback_log_info(
    void (*callback_func_ptr)(unsigned, const char*, va_list arg)) {
  m_callback_log_info = callback_func_ptr;
}

void LogForwarder<void (*)(unsigned, const char*, va_list)>::set_forwarding_callback_log_debug(
    void (*callback_func_ptr)(unsigned, const char*, va_list arg)) {
  m_callback_log_debug = callback_func_ptr;
}

void LogForwarder<void (*)(unsigned, const char*, va_list)>::set_forwarding_callback_log_trace(
    void (*callback_func_ptr)(unsigned, const char*, va_list arg)) {
  m_callback_log_trace = callback_func_ptr;
}

void LogForwarder<void (*)(unsigned, const char*, va_list)>::set_forwarding_callback_log_warn(
    void (*callback_func_ptr)(unsigned, const char*, va_list arg)) {
  m_callback_log_warn = callback_func_ptr;
}

void LogForwarder<void (*)(unsigned, const char*, va_list)>::set_forwarding_callback_log_error(
    void (*callback_func_ptr)(unsigned, const char*, va_list arg)) {
  m_callback_log_error = callback_func_ptr;
}

void LogForwarder<void (*)(unsigned, const char*, va_list)>::clear_forwarding_callback_log_info() {
  m_callback_log_info = 0;
}

void LogForwarder<void (*)(unsigned, const char*, va_list)>::clear_forwarding_callback_log_debug() {
  m_callback_log_debug = 0;
}

void LogForwarder<void (*)(unsigned, const char*, va_list)>::clear_forwarding_callback_log_trace() {
  m_callback_log_trace = 0;
}

void LogForwarder<void (*)(unsigned, const char*, va_list)>::clear_forwarding_callback_log_warn() {
  m_callback_log_warn = 0;
}

void LogForwarder<void (*)(unsigned, const char*, va_list)>::clear_forwarding_callback_log_error() {
  m_callback_log_error = 0;
}

void LogForwarder<void (*)(unsigned, const char*, va_list)>::set_arg0_log_info(
    const unsigned arg0) {
  m_arg0_log_info = arg0;
}

void LogForwarder<void (*)(unsigned, const char*, va_list)>::set_arg0_log_debug(
    const unsigned arg0) {
  m_arg0_log_debug = arg0;
}

void LogForwarder<void (*)(unsigned, const char*, va_list)>::set_arg0_log_trace(
    const unsigned arg0) {
  m_arg0_log_trace = arg0;
}

void LogForwarder<void (*)(unsigned, const char*, va_list)>::set_arg0_log_warn(
    const unsigned arg0) {
  m_arg0_log_warn = arg0;
}

void LogForwarder<void (*)(unsigned, const char*, va_list)>::set_arg0_log_error(
    const unsigned arg0) {
  m_arg0_log_error = arg0;
}

} // namespace OCPIProjects
