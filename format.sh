#!/bin/bash
find ./ -name "*.h" | xargs astyle
find ./ -name "*.cpp" | xargs astyle
find ./ -name "*.c" | xargs astyle
find ./ -name "*.c++" | xargs astyle
find ./ -name "*.orig" | xargs rm -f
