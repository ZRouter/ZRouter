#!/bin/sh


echo ------------------------------------------------------------------------------------------------
echo ------------------------------------------------------------------------------------------------
echo ------------------------------------------------------------------------------------------------
echo ------------------------------------------------------------------------------------------------

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

echo "Delete non existing ARRAY child"
TEST='./conf -S -D -f hash.json glossary.GlossDiv.GlossList.5'

echo ${TEST}
${TEST}

echo "Delete non existing HASH child"
TEST='./conf -S -D -f hash.json glossary.nonexisting'

echo ${TEST}
${TEST}

