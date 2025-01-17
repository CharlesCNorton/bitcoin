#!/usr/bin/env bash
#
# Copyright (c) 2019-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export HOST=x86_64-apple-darwin
# Homebrew's python@3.12 is marked as externally managed (PEP 668).
# Therefore, `--break-system-packages` is needed.
export PIP_PACKAGES="--break-system-packages zmq"
export GOAL="install"
# -Wno-error=deprecated-declarations is to disable errors for deprecation warnings:
# "'NSUserNotificationCenter' is deprecated: first deprecated in macOS 11.0".
export BITCOIN_CONFIG="--with-gui --with-miniupnpc --with-natpmp --enable-reduce-exports CXXFLAGS=-Wno-error=deprecated-declarations"
export CI_OS_NAME="macos"
export NO_DEPENDS=1
export OSX_SDK=""
export CCACHE_MAXSIZE=400M
export RUN_FUZZ_TESTS=true
export FUZZ_TESTS_CONFIG="--exclude banman"  # https://github.com/bitcoin/bitcoin/issues/27924
