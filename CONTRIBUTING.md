A Guide for Contributors
========================

Contributors must agree to the [Developer Certificate of Origin (DCO)](https://developercertificate.org/). You should indicate compliance by adding the following to each commit (using `git commit --signoff`):

```
Signed-off-by: NAME <EMAIL>
```

When opening a merge request, please enable this option:

> [ ] Allow commits from members who can merge to the target branch.

This will allow committers to do simple fixups to your commits, and to
rebase them prior to merging.

A Guide for Committers
======================

* All patches must be reviewed before pushing, with some exceptions:
  - Fixing breakage such as dead URLs
  - Trivial changes (obvious updates)
* The repository should have no merge commits

Reviewers should use the following workflow to merge a merge request:

* Review it!
* Pull the branch locally
* Rebase onto the top of master
* Force-push to the contributor's branch
* Approve on the web interface
* Merge through the web interface
