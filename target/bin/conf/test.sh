#!/bin/sh

TEST='./conf -S -f hash.json glossary.title glossary.GlossDiv.GlossList.1 glossary.GlossDiv.GlossList.2=yyy'

echo ${TEST}
${TEST}

echo "Tree overwrite, should ignore while user delete childs"
TEST='./conf -S -f hash.json glossary.GlossDiv.GlossList.0=yyy'

echo ${TEST}
${TEST}

echo "Tree overwrite, should ignore while user delete childs"
TEST='./conf -S -f hash.json glossary.GlossDiv.GlossList.1=yy1 glossary.GlossDiv.GlossList.2=yy2 glossary.GlossDiv.GlossList.3=yy3'

echo ${TEST}
${TEST}

echo "Delete childs"
TEST='./conf -S -D -f hash.json glossary.GlossDiv.GlossList.1'

echo ${TEST}
${TEST}

