#!/usr/bin/env python3
# This file is protected by Copyright. Please refer to the COPYRIGHT file
# distributed with this source distribution.
#
# This file is part of OpenCPI <http://www.opencpi.org>
#
# OpenCPI is free software: you can redistribute it and/or modify it under the
# terms of the GNU Lesser General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option) any
# later version.
#
# OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
# A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
# details.
#
# You should have received a copy of the GNU Lesser General Public License along
# with this program. If not, see <http://www.gnu.org/licenses/>.


import argparse
import sys
import subprocess
import tempfile
import os
import tarfile
import datetime
import glob
import collections
import textwrap
import _opencpi.util as ocpiutil

class UI():
    """
    User Interface. Defines all the options a user can select on the CLI,
    and calls the one command (verb) that the user selects.

    To add a new command (verb), just define it here in the appropriate
    table. Create a function for it to call, add any new flags needed,
    then associate the flags to the command.

    Example usage:
    ocpiremote.py --help
    ocpiremote.py [options]    <command>  [sub-options]
    ocpiremote.py --user Admin  load      --hw_platform zed --port 99    
    ocpiremote.py               load      --help

    We are using the argparse subparsers for the commands, to avoid
    displaying help for command-specific flags in the main help. This
    leads to the syntax above of
    
    [common-options] <command> [sub-options]
    
    instead of
    
    [every-option] <command>

    """
    def __init__(self):
        
        # Namedtuples! No more cmd[0], now we use cmd.name, etc!
        Cmd       = collections.namedtuple('Cmd',       'name function help cmd_options')
        CmdOption = collections.namedtuple('CmdOption', 'short long default help')

        # Defaults
        user        = 'root'
        passwd      = 'root'
        remote_dir  = 'sandbox'
        hw_platform = 'zed'
        sw_platform = 'xilinx13_4'
        log_level   = '0'
        ssh_opts    = ''
        ip, port    = self._get_address_port(silent=True)
        ip   = str(ip)   if ip   is not None else ''
        port = str(port) if port is not None else ''

        # Colorize and show the defaults in help display
        color = '\033[31m' # RED
        end = '\033[0m'
        _colorize  = lambda word : ''.join([color,word,end])
        _format    = lambda msg, color : msg.format(_colorize(color))
        opt_maker  = lambda short, long, default, help : CmdOption(short,
                                                                   long,
                                                                   default,
                                                                   _format('({}) ' + help, default if default is not None else '' ))

        # Options we will later associate with specific commands, or declare as common/global.
        opt_user        = opt_maker( 'u' , 'user'        , user         , 'Login user')
        opt_passwd      = opt_maker( 'p' , 'passwd'      , passwd       , 'Login password')
        opt_ip_addr     = opt_maker( 'i' , 'ip_addr'     , ip           , 'Remote address. First address in OCPI_SERVER_ADDRESSES')
        opt_port        = opt_maker( 'r' , 'port'        , port         , 'Remote\'s server port. First port in OCPI_SERVER_ADDRESSES')
        opt_ssh_opts    = opt_maker( 'o' , 'ssh_opts'    , ssh_opts     , 'SSH options')
        opt_remote_dir  = opt_maker( 'd' , 'remote_dir'  , remote_dir   , 'Directory on remote to create/use as the server sandbox')
        opt_sw_platform = opt_maker( 's' , 'sw_platform' , sw_platform  , 'Forms part of path under OCPI_CDK_DIR to create server package')
        opt_hw_platform = opt_maker( 'w' , 'hw_platform' , hw_platform  , 'Forms part of path under OCPI_CDK_DIR to create server package')
        opt_valgrind    = opt_maker( 'v' , 'valgrind'    , None         , 'Load/Use Valgrind')
        opt_log_level   = opt_maker( 'l' , 'log_level'   , log_level    , 'Specify a log level to use with \'start\'')
        opt_help        = opt_maker( 'h' , 'help'        , None         , 'Show this help message and exit')

        # Actual command/verbs to call (one at a time), and their options.
        cmds = []
        cmds.append( Cmd( 'test'   , do_cmd_test   , 'Test basic connectivity'                                           , ()                                        ))
        cmds.append( Cmd( 'load'   , do_cmd_load   , 'Create and send the server package to the remote sandbox directory', (opt_sw_platform, opt_hw_platform, opt_port, opt_valgrind)))
        cmds.append( Cmd( 'start'  , do_cmd_start  , 'Calls \'./ocpiserver.sh start\'  on the remote'                    , (opt_log_level,opt_valgrind)              ))
        cmds.append( Cmd( 'status' , do_cmd_status , 'Calls \'./ocpiserver.sh status\' on the remote'                    , ()                                        ))
        cmds.append( Cmd( 'stop'   , do_cmd_stop   , 'Calls \'./ocpiserver.sh stop\'   on the remote'                    , ()                                        ))
        cmds.append( Cmd( 'unload' , do_cmd_unload , 'Delete a server sandbox directory'                                 , ()                                        ))

        # Ok, this is everything we care about:
        self.cmds = cmds
        self.common_options = (opt_user, opt_passwd, opt_ip_addr, opt_remote_dir, opt_ssh_opts, opt_help)

    def _get_address_port(self, silent=False):
        address = None
        port    = None
        addresses = os.getenv('OCPI_SERVER_ADDRESSES')
        if addresses:
            if len(addresses.split()) > 1:
                ocpiutil.logging.warning('Will only use FIRST ip_addr from OCPI_SERVER_ADDRESSES')
            try:
                address, port = addresses.split()[0].split(':')
            except IndexError:
                if not silent:
                    raise BenignException('OCPI_SERVER_ADDRESSES is not formatted correctly')
        return address, port

    def _add_flags(self, flags, parent):
        for option in flags:
            if option.default is None:
                # Bool type flags that don't accept a value, e.g.
                # --valgrind, --help
                parent.add_argument('-'    + option.short,
                                    '--'   + option.long,
                                    help   = option.help,
                                    action = 'store_true')
            else:
                # Flags that need a value, e.g.
                # --user Admin
                parent.add_argument('-'     + option.short,
                                    '--'    + option.long,
                                    help    = option.help,
                                    default = option.default)

    def get_opts(self):
        """
        Setup our argument parser, get the user's input, then return a
        dictionary-like object of their choices.
        """

        desc = textwrap.dedent('''\
            Requires {bld}OCPI_CDK_DIR{end} to be set.

            Loads a minimal server installation of OpenCPI in a sandbox directory
            on a remote device, which can then be controlled with further commands,
            e.g. START, STOP. Does not affect the SD Card installation. The server
            package is pulled from the OCPI_CDK_DIR directory, based on the
            HW_PLATFORM and SW_PLATFORM values specified with the LOAD command.'''.format(bld='\033[01m', end='\033[0m'))
        
        epil = textwrap.dedent('''\
            {bld}Example:{end}
                Enable detailed logging, set defaults, use Zed, Xilinx13_3, and Valgrind:

                export OCPI_LOG_LEVEL=10
                ./ocpiremote.py -i <IP_ADDR> test

                export OCPI_SERVER_ADDRESSES=<IP_ADDR>:<PORT>
                ./ocpiremote.py test
                ./ocpiremote.py load  -v -w zed -s xilinx13_3
                ./ocpiremote.py start -v -l 8
                ./ocpiremote.py status
                ./ocpiremote.py stop
                ./ocpiremote.py unload'''.format(bld='\033[01m', end='\033[0m'))

        # Main Parser
        parser = argparse.ArgumentParser(description=desc, 
                                         prog='ocpiremote.py',
                                         usage='%(prog)s [options] <command> [sub-options]', 
                                         epilog=epil,
                                         formatter_class=argparse.RawDescriptionHelpFormatter,
                                         add_help=False)
        
        # Common Options/Flags (not command specific)
        common_group = parser.add_argument_group('{bld}Common Options{end}'.format(bld='\033[01m',
                                                                                   end='\033[0m'),
                                                 'Prefix to all commands')
        
        # Populate them...
        self._add_flags(self.common_options, common_group)
        
        # Actual Commands
        subparsers = parser.add_subparsers(title='{bld}Commands{end}'.format(bld='\033[01m',
                                                                             end='\033[0m'),
                                           description='See sub-options with <command> --help',
                                           help='',
                                           dest='cmd',
                                           metavar='')

        # Populate them...
        for cmd in self.cmds:
            sub = subparsers.add_parser(cmd.name,
                                        help  = cmd.help,
                                        prog  = 'ocpiremote.py [options]',
                                        usage ='%(prog)s {0} [{0}-options]'.format(cmd.name),
                                        formatter_class=argparse.RawDescriptionHelpFormatter )

            sub.set_defaults(func=cmd.function)
            self._add_flags(cmd.cmd_options, sub)
        
        # We will return a simple dictionary-like object, e.g.
        # retval.cmd     = 'load'
        # retval.user    = 'root'
        # retval.port    = 97
        # retval.func    = <function object do_cmd_load 0x1A35FSA> 
        
        retval, fluff = parser.parse_known_args()
        
        # Check for basic sanity
        if fluff:
            msg = 'Invalid options: {}'.format(' '.join(fluff))
            parser.print_usage()
            raise BenignException(msg)
        
        if not retval.cmd and retval.help:
            parser.print_help()
            raise BenignException('')
        
        if not retval.cmd:
            parser.print_help()
            raise BenignException('a command is required')

        if '..' in retval.remote_dir:
            raise BenignException('remote_dir cannot contain relative paths')

        if retval.remote_dir.startswith('/'):
            raise BenignException('remote_dir cannot start with a \'/\'')

        if not retval.ip_addr:
            ocpiutil.logging.warning('Using first ip_addr:port in OCPI_SERVER_ADDRESSES')

            retval.ip_addr, retval.port = self._get_address_port()
            if not retval.ip_addr:            
                raise BenignException('ip_addr is required')

        if retval.cmd == 'load' and not retval.port:
            raise BenignException('port is required')

        return retval

class Remote():
    """
    Utilities for interacting with remote host.
    """
 
    _basic_sshopts = (    ' '
                        + '-o StrictHostKeyChecking=no '
                        + '-o GlobalKnownHostsFile=/dev/null '
                        + '-o UserKnownHostsFile=/dev/null '
                        + '-o ConnectTimeout=10 ' )

    @classmethod
    def _executeCmd(cls, in_command, opts):
        """
        Execute an ssh or scp command in a subprocess. Takes care of 
        setting up a temp password file (in a temp directory), and 
        the environment needed by SSH/SCP to use it.
        """
        # We are doing ssh/scp without key exchange and without
        # presenting a password prompt, so there are hoops to jump through.
        # Use 'with' context to ensure password file's containing directory
        # is always deleted.
        with tempfile.TemporaryDirectory() as tmpdir:

            passwd_file_path = ''

            # Use the ssh SSH_ASKPASS feature: Create a temp file that will
            # echo the password when executed.
            with tempfile.NamedTemporaryFile(dir=tmpdir, mode='w+b', buffering=0, delete=False) as passwd_file:
                passwd_file_path = passwd_file.name
                passwd_file.write(bytes('echo ' + opts.passwd, 'utf-8'))
                os.chmod(passwd_file_path, 0o700)

            try:
                process = subprocess.Popen( in_command.split(),
                                            env={'SSH_ASKPASS': passwd_file_path, 'DISPLAY': 'DUMMY'},
                                            start_new_session=True,
                                            stdout=subprocess.PIPE,
                                            stderr=subprocess.PIPE )

                stdout_data, stderr_data = process.communicate()
                stdout = str(stdout_data.decode('utf-8')).strip()
                stderr = str(stderr_data.decode('utf-8')).strip()
                
                return (process.returncode, stdout, stderr)

            except Exception as e:
                raise ocpiutil.OCPIException('SSH/SCP call failed in a way we cannot handle; quitting. {}'.format(e))

    @classmethod
    def send_file(cls, in_file_path, opts):
        """
        Send a file to remote host using SCP.
        """
        sshopts = opts.ssh_opts + ' ' + cls._basic_sshopts

        command = 'scp {sshopts} {file_path} {opts.user}@{opts.ip_addr}:./{opts.remote_dir}'.format(sshopts   = sshopts,
                                                                                                    file_path = in_file_path,
                                                                                                    opts = opts)
        ocpiutil.logging.debug('Sending {} To Remote'.format(in_file_path))
        return cls._executeCmd(command, opts)

    @classmethod
    def send_cmds(cls, commands, opts):
        """
        commands is sent to "sh -c" with no additional parsing or escaping.
        """

        if '\\"' in commands:
            raise BenignException('Escaped " are not allowed.')

        sshopts = opts.ssh_opts + ' -x -T ' + cls._basic_sshopts

        command = 'ssh {sshopts} {opts.user}@{opts.ip_addr} sh -c "{commands}"'.format(sshopts  = sshopts,
                                                                                       commands = commands,
                                                                                       opts     = opts)
        return cls._executeCmd(command, opts)


def _call_ocpi_server( ocpi_server_cmd, opts ):
    """Helper that calls simple verbs on the remote's ocpiserver.sh script"""

    cmd = 'cd {remote_dir} && ./ocpiserver.sh {ocpi_server_cmd}'.format( ocpi_server_cmd=ocpi_server_cmd, remote_dir=opts.remote_dir )
    return_code, stdout, stderr = Remote.send_cmds(cmd, opts)
    if return_code != 0:
        raise BenignException('ocpiserver.sh failed \'{}\'. stdout: {} stderr: {}'.format(ocpi_server_cmd, stdout, stderr))


def do_cmd_start(opts):
    args = ''
    if opts.valgrind:
        args += ' -V '
    if int(opts.log_level) > 0:
        args += ' -l {} '.format(opts.log_level)
    args += 'start'
    _call_ocpi_server( args, opts )

def do_cmd_stop( opts ):
    _call_ocpi_server( 'stop'   , opts )

def do_cmd_stop_if( opts ):
    _call_ocpi_server( 'stop_if', opts )

def do_cmd_log( opts ):
    _call_ocpi_server( 'log'    , opts )

def do_cmd_status(opts):
    _call_ocpi_server( 'status' , opts )

def do_cmd_not_implemented(opts):
    ocpiutil.logging.debug('Called \'not_implemented\' with {}'.format(opts))
    raise BenignException('This command is not implemented')

def do_cmd_test(opts):
    """ Check for connectivity by running "pwd" on remote host"""
    retcode, stdout, _ = Remote.send_cmds('pwd', opts)
    if retcode != 0:
        raise BenignException('Executing \'pwd\' on remote failed')
    ocpiutil.logging.debug(stdout)

def do_cmd_unload(opts):
    """ Deletes the remote_dir on remote host."""
    
    ocpiutil.logging.debug('Removing \'{}\' from remote host'.format(opts.remote_dir))
    return_code, stdout, stderr = Remote.send_cmds('rm -rf {opts.remote_dir}'.format(opts=opts), opts)
    if return_code != 0:
        raise ocpiutil.OCPIException('Failed. stdout: {} stderr: {}'.format(stdout, stderr))

def do_cmd_load(opts):
    """
    The main functionality is at the bottom of this function, where we:

    1) PRE-CMDS:  Prep remote device.
    2) SEND-TAR:  Create and send a tar file of everything needed.
    3) POST-CMDS: Extract tar on remote, etc.
    """
    
    # COMMANDS TO SEND:
    cmds_pre  = [] # Send before tar file.
    cmds_post = [] # Send after tar file

    # Note: set the date in the format allowed by the most devices.
    date_setter = lambda: 'date {}'.format(datetime.datetime.now().strftime('%m%d%H%M%Y.%S'))

    Cmd = collections.namedtuple('Cmd', ['note', 'cmd', 'successful_return_code'])
    
    # We use a helper function below, to call these, so the cmds can be
    # either a simple string, or a function that returns a string.
    #                    Note                   Command                                            Successful Return Code
    cmds_pre.append( Cmd('Check remote_dir'    ,'test -e {opts.remote_dir}'                             .format(opts=opts) , 1 ))
    cmds_pre.append( Cmd('Create remote_dir'   ,'mkdir {opts.remote_dir}'                               .format(opts=opts) , 0 ))
    cmds_pre.append( Cmd('Set date'            , date_setter                                                               , 0 ))
    cmds_pre.append( Cmd('Set platfrom'        ,'echo {opts.sw_platform} > {opts.remote_dir}/swplatform'.format(opts=opts) , 0 ))
    cmds_pre.append( Cmd('Set port'            ,'echo {opts.port}        > {opts.remote_dir}/port'      .format(opts=opts) , 0 ))
    cmds_pre.append( Cmd('Link ocpiserver.sh'  ,'ln -s scripts/ocpiserver.sh {opts.remote_dir}'         .format(opts=opts) , 0 ))
    cmds_pre.append( Cmd('Link system.xml'     ,'ln -s {opts.sw_platform}/system.xml {opts.remote_dir}' .format(opts=opts) , 0 ))

    cmds_post.append( Cmd('Extract tar' ,'tar -x -z -C {opts.remote_dir} -f {opts.remote_dir}/tar.tgz'  .format(opts=opts) , 0 ))
    cmds_post.append( Cmd('Set time zone (does this even work?)', 'export TZ=`pwd`/{opts.remote_dir}/etc/localtime'.format(opts=opts) , 0 ))

    # FILES TO PUT INTO TAR:

    # We have to take care of files from three situations for the tar file
    # to be correct:
    # 1) Simple files (with optional * wildcards) we can just tar up,
    #    relative to the cdk_platform_path (defined below).
    # 2) A file called "driver-list" that provides a LIST of files we
    #    also need.
    # 3) Files in the CDK directory that need their relative paths changed,
    #    unfortunately, to satisfy the scripts used on the remote device.

    cdk_platform_path = '{cdk}/{opts.hw_platform}/sdcard-{opts.sw_platform}/opencpi'.format(cdk=os.environ['OCPI_CDK_DIR'], opts=opts)
    if not os.path.exists(cdk_platform_path):
        raise ocpiutil.OCPIException('Path does not exist: {}'.format(cdk_platform_path))

    # 1) Simple files with optional * wildcards
    tar_me_wildcards =  [
        '/etc/localtime',
        'scripts/ocpi_linux_driver',
        '{}/lib/*.ko'      .format(opts.sw_platform),
        '{}/lib/*.rules'   .format(opts.sw_platform),
        '{}/bin/ocpidriver'.format(opts.sw_platform),
        '{}/bin/ocpiserve' .format(opts.sw_platform),
        '{}/bin/gdb'       .format(opts.sw_platform)
    ]

    # 2) driver-list
    driver_list_file = '{}/{}/lib/driver-list'.format(cdk_platform_path, opts.sw_platform)

    with open(driver_list_file) as fd:
        drivers = [driver for line in fd for driver in line.strip().split()]
        tar_me_wildcards += list(map(lambda driver: '{}/lib/libocpi_{}_s.so'.format(opts.sw_platform, driver), drivers))

    # 3) Relative paths need changing
    #                   Source Path                         Target Path in TAR file
    tar_me_relative = [('../../../scripts/ocpiserver.sh', 'scripts/ocpiserver.sh'),
                       ('system.xml'                     , '{}/system.xml'.format(opts.sw_platform))]
    if opts.valgrind:
        tar_me_relative.append(('../../../../prerequisites/valgrind/{}'.format(opts.sw_platform), 'prerequisites/valgrind/{}'.format(opts.sw_platform)))


    def _send_cmds(cmds):
        """
        Helper to actually send commands to remote. The command
        can be a function or a simple string; which is really just for
        setting the date. Without some approach like this, we end up 
        with the time being ~10 seconds behind on remote device.
        """
        for cmd in cmds:
            cmd_str = cmd.cmd() if callable(cmd.cmd) else cmd.cmd # function or simple string
            ocpiutil.logging.debug(cmd_str)
            return_code, stdout, stderr = Remote.send_cmds(cmd_str, opts)
            if cmd.successful_return_code != return_code:
                raise ocpiutil.OCPIException('Failed \'{}\'. cmd: {} stdout: {} stderr: {}'.format(cmd.note, cmd_str, stdout, stderr))

    def _make_tar(in_tar_me_wildcards, in_tar_me_relative, in_temp_dir):
        """
        Helper to create a tar file. Assumes correct cwd.
        A particular device requires PAX_FORMAT.
        """
        # Expand wildcards
        tar_me_wildcards = []
        for expanded_wildcards in map(glob.glob, in_tar_me_wildcards):
            if not expanded_wildcards:
                raise ocpiutil.OCPIException('Could not find all required files after expanding * wildcards!')
            tar_me_wildcards += expanded_wildcards

        # Now actually create the tar file
        tar_path = in_temp_dir + '/tar.tgz'
        with tarfile.open(tar_path, 'w:gz', format=tarfile.PAX_FORMAT, dereference=True) as tar: # Dereference for localtime's link

            def _fix_permissions(tar_info_obj):
                """ Permissions on some files aren't consistent, so we fix."""
                tar_info_obj.mode = tar_info_obj.mode | 0o744
                return tar_info_obj

            for path_on_disk in tar_me_wildcards:
                tar.add(path_on_disk, filter=_fix_permissions)
            for path_on_disk, path_in_tar in in_tar_me_relative:
                tar.add(path_on_disk, filter=_fix_permissions, arcname=path_in_tar,)

        return tar_path

    # MAIN FUNCTIONALITY

    # The tar creation step (and its wildcard expansion) below requires us to be in
    # this directory. The rest of the steps do not, but it's just easier
    # to do it all in the same context.
    with ocpiutil.cd(cdk_platform_path):

        # Now we can create the TAR file, then...
        with tempfile.TemporaryDirectory() as temp_dir:

            tar_file = _make_tar(tar_me_wildcards, tar_me_relative, temp_dir)

            # PRE-CMDS
            _send_cmds(cmds_pre)

            # SEND-TAR
            return_code, stdout, stderr = Remote.send_file(tar_file, opts)
            if return_code != 0:
                raise ocpiutil.OCPIException('Could not send tar file to remote. stdout: {} stderr: {}'.format(stdout, stderr))

        # POST-CMDS
        _send_cmds(cmds_post)

class BenignException(ocpiutil.OCPIException):
    pass

def main():

    try:
        # Setup all our CLI options the user can choose.
        ui = UI()

        # Get dictionary-like object of user choices
        opts = ui.get_opts()
        
        # User performs one cmd at a time, we have it now!
        # So call it, passing in all their choices.
        opts.func(opts)

    except BenignException as e:
        if len(str(e)) > 0:
            ocpiutil.logging.warning('{}'.format(e))

if __name__ == '__main__':
    main()
