#!/bin/bash

if ! git remote | grep -q "^upstream$"; then
    git remote add upstream https://github.com/crosspoint-reader/crosspoint-reader.git
fi

git fetch upstream
git merge upstream/master
