#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Copyright © 2018 Endless Mobile, Inc.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this library; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA

import argparse
import os
import pwd
import sys
import gi
gi.require_version('EosParentalControls', '0')  # noqa
from gi.repository import EosParentalControls, GLib


# Exit codes, which are a documented part of the API.
EXIT_SUCCESS = 0
EXIT_INVALID_OPTION = 1
EXIT_PERMISSION_DENIED = 2
EXIT_PATH_NOT_ALLOWED = 3


def __get_app_filter(user_id, interactive):
    """Get the app filter for `user_id` off the bus.

    If `interactive` is `True`, interactive polkit authorisation dialogues will
    be allowed. An exception will be raised on failure."""
    app_filter = None
    exception = None

    def __get_cb(obj, result, user_data):
        nonlocal app_filter, exception
        try:
            app_filter = EosParentalControls.get_app_filter_finish(result)
        except Exception as e:
            exception = e

    EosParentalControls.get_app_filter_async(
        connection=None, user_id=user_id,
        allow_interactive_authorization=interactive, cancellable=None,
        callback=__get_cb, user_data=None)

    context = GLib.MainContext.default()
    while not app_filter and not exception:
        context.iteration(True)

    if exception:
        raise exception
    return app_filter


def __get_app_filter_or_error(user_id, interactive):
    """Wrapper around __get_app_filter() which prints an error and raises
    SystemExit, rather than an internal exception."""
    try:
        return __get_app_filter(user_id, interactive)
    except GLib.Error as e:
        print('Error getting app filter for user {}: {}'.format(
            user_id, e.message), file=sys.stderr)
        raise SystemExit(EXIT_PERMISSION_DENIED)


def __lookup_user_id(user):
    """Convert a command-line specified username or ID into a user ID. If
    `user` is empty, use the current user ID.

    Raise KeyError if lookup fails."""
    if user == '':
        return os.getuid()
    elif user.isdigit():
        return int(user)
    else:
        return pwd.getpwnam(user).pw_uid


def __lookup_user_id_or_error(user):
    """Wrapper around __lookup_user_id() which prints an error and raises
    SystemExit, rather than an internal exception."""
    try:
        return __lookup_user_id(user)
    except KeyError:
        print('Error getting ID for username {}'.format(user), file=sys.stderr)
        raise SystemExit(EXIT_INVALID_OPTION)


def command_get(user, quiet=False, interactive=True):
    """Get the app filter for the given user."""
    user_id = __lookup_user_id_or_error(user)
    __get_app_filter_or_error(user_id, interactive)

    print('App filter for user {} retrieved'.format(user_id))


def command_check(user, path, quiet=False, interactive=True):
    """Check the given path is runnable by the given user, according to their
    app filter."""
    user_id = __lookup_user_id_or_error(user)
    app_filter = __get_app_filter_or_error(user_id, interactive)

    path = os.path.abspath(path)

    if app_filter.is_path_allowed(path):
        print('Path {} is allowed by app filter for user {}'.format(
            path, user_id))
        return
    else:
        print('Path {} is not allowed by app filter for user {}'.format(
            path, user_id))
        raise SystemExit(EXIT_PATH_NOT_ALLOWED)


def main():
    # Parse command line arguments
    parser = argparse.ArgumentParser(
        description='Query and update parental controls.')
    subparsers = parser.add_subparsers(metavar='command',
                                       help='command to run (default: ‘get’)')
    parser.set_defaults(function=command_get)
    parser.add_argument('-q', '--quiet', action='store_true',
                        help='output no informational messages')
    parser.set_defaults(quiet=False)

    # Common options for the subcommands which might need authorisation.
    common_parser = argparse.ArgumentParser(add_help=False)
    group = common_parser.add_mutually_exclusive_group()
    group.add_argument('-n', '--no-interactive', dest='interactive',
                       action='store_false',
                       help='do not allow interactive polkit authorization '
                            'dialogues')
    group.add_argument('--interactive', dest='interactive',
                       action='store_true',
                       help='opposite of --no-interactive')
    common_parser.set_defaults(interactive=True)

    # ‘get’ command
    parser_get = subparsers.add_parser('get', parents=[common_parser],
                                       help='get current parental controls '
                                            'settings')
    parser_get.set_defaults(function=command_get)
    parser_get.add_argument('user', default='', nargs='?',
                            help='user ID or username to get the app filter '
                                 'for (default: current user)')

    # ‘check’ command
    parser_check = subparsers.add_parser('check', parents=[common_parser],
                                         help='check whether a path is '
                                              'allowed by app filter')
    parser_check.set_defaults(function=command_check)
    parser_check.add_argument('user', default='', nargs='?',
                              help='user ID or username to get the app filter '
                                   'for (default: current user)')
    parser_check.add_argument('path',
                              help='path to a program to check')

    # Parse the command line arguments and run the subcommand.
    args = parser.parse_args()
    args_dict = dict((k, v) for k, v in vars(args).items() if k != 'function')
    args.function(**args_dict)


if __name__ == '__main__':
    main()
