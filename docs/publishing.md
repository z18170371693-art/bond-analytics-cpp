# Publishing in the project owner's account

The public project is at
https://github.com/z18170371693-art/bond-analytics-cpp. Its workflow checks
the build, tests, benchmark, and Doxygen on three platforms and publishes the
Doxygen site after the full matrix succeeds.

## Publish a separate copy

Install [GitHub CLI](https://cli.github.com/) and Git, then authenticate locally:

```sh
gh auth login
gh auth status
bash scripts/publish_github.sh
```

The script verifies that the locally authenticated account is
`z18170371693-art`, creates a public repository if the target does not exist,
updates README links, commits the project, pushes `main`, and tries to configure
GitHub Pages to build from Actions. Its local GitHub CLI login is separate from
the ChatGPT/GitHub connection and must be authenticated independently.

If the repository already exists, it must be public and its `origin` must match
that repository. The script will not make a private repository public, replace a
different remote, or force-push unrelated history.

## Verify the Actions run

Open the [Actions page](https://github.com/z18170371693-art/bond-analytics-cpp/actions).
The workflow builds on Ubuntu, macOS, and Windows, runs CTest and numerical
oracles, prints benchmark timings, validates JSON, and checks Doxygen links.
Artifacts retain each platform's benchmark JSON and generated HTML. The Pages
job runs after the complete build matrix succeeds on a push to the default
branch. Pull requests run validation but do not publish.

All platform jobs and the Pages deployment must be green. Read the logs for the
test count and benchmark validation message, download a measurement artifact,
and open the published documentation link from README. If a job fails, fix the
cause and push another commit; do not label a failed or unrun workflow as passed.

If Pages is not enabled, open **Settings → Pages** and choose **GitHub Actions**
as the build and deployment source. After changing the setting, rerun the
workflow if its first deployment failed before Pages was enabled. The generated
documentation URL is usable only after a successful deployment.

This workflow uses GitHub's official actions:
[checkout](https://github.com/actions/checkout),
[upload-artifact](https://github.com/actions/upload-artifact),
[upload-pages-artifact](https://github.com/actions/upload-pages-artifact), and
[deploy-pages](https://github.com/actions/deploy-pages). GitHub's
[Pages workflow documentation](https://docs.github.com/en/pages/getting-started-with-github-pages/using-custom-workflows-with-github-pages)
describes the deployment permissions and settings.

## Submit

Keep the public repository link, successful Actions run link, and actual
documentation link. Record the final commit hash so the submitted files and
public evidence refer to the same version. Local checks are not evidence of
GitHub CI success or a public deployment.
