#!/bin/sh

set -e

git remote add dcocheck https://gitlab.com/libosinfo/osinfo-db.git
git fetch dcocheck
ancestor=`git merge-base dcocheck/master HEAD`
git remote rm dcocheck

status=0

echo "Check commits since $ancestor..."
echo

for sha in `git log --format=%H $ancestor..`
do
    set +e
    git show -s $sha | grep "Signed-off-by:" >/dev/null
    missing=$?
    set -e

    if test $missing != 0
    then
	echo "ERROR: Missing 'Signed-off-by' on commit $sha"
	status=1
    fi
done

if test $status != 0
then
    cat <<EOF

This project requires all contributors to assert that
contributions are in compliance with the terms of the
Developer's Certificate of Origin 1.1  (DCO)

  https://developercertificate.org/

To indicate compliance every commit must have a tag

  Signed-off-by: REAL NAME <EMAIL>

This can be achieved by passing the "-s" flag to the
"git commit" command.

To update all commits on the current branch to add a
Signed-off-by tag, use

  git rebase -i master -x 'git commit --amend --no-edit -s'

EOF

fi

exit $status
