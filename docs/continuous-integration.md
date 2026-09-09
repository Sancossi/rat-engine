# Repository CI

[The workflow](../.github/workflows/ci.yml) runs on push and pull requests with
read-only repository permissions. Its Ubuntu 24.04 job installs Python 3.13 and
runs vault validation and the Python unit tests within a 30-minute timeout.
These cover vault/BMad contracts and Stride checkout guards.

Full Stride builds and executable acceptance currently use the local Windows
[game workflow](../games/rat-expedition/README.md) and
[source/editor workflow](stride-source-workflow.md). Adding hosted Stride build,
GPU or package jobs is separate work. Configuration and local success do not
establish an observed remote CI run.

The retired C++ build/GUI/sanitizer/static-analysis workflow is documented in the
[C++ archive](archive/cpp/continuous-integration.md); its jobs and scripts are no
longer active.
