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


def __set_app_filter(user_id, app_filter, interactive):
    """Set the app filter for `user_id` off the bus.

    If `interactive` is `True`, interactive polkit authorisation dialogues will
    be allowed. An exception will be raised on failure."""
    finished = False
    exception = None

    def __set_cb(obj, result, user_data):
        nonlocal finished, exception
        try:
            EosParentalControls.set_app_filter_finish(result)
            finished = True
        except Exception as e:
            exception = e

    EosParentalControls.set_app_filter_async(
        connection=None, user_id=user_id, app_filter=app_filter,
        allow_interactive_authorization=interactive, cancellable=None,
        callback=__set_cb, user_data=None)

    context = GLib.MainContext.default()
    while not finished and not exception:
        context.iteration(True)

    if exception:
        raise exception


def __set_app_filter_or_error(user_id, app_filter, interactive):
    """Wrapper around __set_app_filter() which prints an error and raises
    SystemExit, rather than an internal exception."""
    try:
        __set_app_filter(user_id, app_filter, interactive)
    except GLib.Error as e:
        print('Error setting app filter for user {}: {}'.format(
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


oars_value_mapping = {
    EosParentalControls.AppFilterOarsValue.UNKNOWN: "unknown",
    EosParentalControls.AppFilterOarsValue.NONE: "none",
    EosParentalControls.AppFilterOarsValue.MILD: "mild",
    EosParentalControls.AppFilterOarsValue.MODERATE: "moderate",
    EosParentalControls.AppFilterOarsValue.INTENSE: "intense",
}


def __oars_value_to_string(value):
    """Convert an EosParentalControls.AppFilterOarsValue to a human-readable
    string."""
    try:
        return oars_value_mapping[value]
    except KeyError:
        return "invalid (OARS value {})".format(value)


def __oars_value_from_string(value_str):
    """Convert a human-readable string to an
    EosParentalControls.AppFilterOarsValue."""
    for k, v in oars_value_mapping.items():
        if v == value_str:
            return k
    raise KeyError('Unknown OARS value ‘{}’'.format(value_str))


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


def command_oars_section(user, section, quiet=False, interactive=True):
    """Get the value of the given OARS section for the given user, according
    to their OARS filter."""
    user_id = __lookup_user_id_or_error(user)
    app_filter = __get_app_filter_or_error(user_id, interactive)

    value = app_filter.get_oars_value(section)
    print('OARS section ‘{}’ for user {} has value ‘{}’'.format(
        section, user_id, __oars_value_to_string(value)))


def command_set(user, app_filter_args=None, quiet=False, interactive=True):
    """Set the app filter for the given user."""
    user_id = __lookup_user_id_or_error(user)
    builder = EosParentalControls.AppFilterBuilder.new()

    for arg in app_filter_args:
        if '=' in arg:
            [section, value_str] = arg.split('=', 2)
            try:
                value = __oars_value_from_string(value_str)
            except KeyError:
                print('Unknown OARS value ‘{}’'.format(value_str),
                      file=sys.stderr)
                raise SystemExit(EXIT_INVALID_OPTION)
            builder.set_oars_value(section, value)
        else:
            builder.blacklist_path(arg)
    app_filter = builder.end()

    __set_app_filter_or_error(user_id, app_filter, interactive)

    print('App filter for user {} set'.format(user_id))


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

    # ‘oars-section’ command
    parser_oars_section = subparsers.add_parser('oars-section',
                                                parents=[common_parser],
                                                help='get the value of a '
                                                     'given OARS section')
    parser_oars_section.set_defaults(function=command_oars_section)
    parser_oars_section.add_argument('user', default='', nargs='?',
                                     help='user ID or username to get the '
                                          'OARS filter for (default: current '
                                          'user)')
    parser_oars_section.add_argument('section', help='OARS section to get')

    # ‘set’ command
    parser_set = subparsers.add_parser('set', parents=[common_parser],
                                       help='set current parental controls '
                                            'settings')
    parser_set.set_defaults(function=command_set)
    parser_set.add_argument('user', default='', nargs='?',
                            help='user ID or username to get the app filter '
                                 'for (default: current user)')
    parser_set.add_argument('app_filter_args', nargs='*',
                            help='paths to blacklist and OARS section=value '
                                 'pairs to store')

    # Parse the command line arguments and run the subcommand.
    args = parser.parse_args()
    args_dict = dict((k, v) for k, v in vars(args).items() if k != 'function')
    args.function(**args_dict)


if __name__ == '__main__':
    main()
