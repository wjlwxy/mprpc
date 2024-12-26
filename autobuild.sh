#!/bin/bash

set -e

rm -rf ${PWD}/build/*  # or ${PWD}/build/*
cd ${PWD}/build &&
    cmake .. &&
    make
cd ..
cp -r ${PWD}/src/include ${PWD}/lib  # 个人感觉意义不大做这一步复制