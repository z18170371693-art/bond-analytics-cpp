# Pinned dependencies

CMake downloads these exact upstream tags during configuration and checks the
SHA-256 digest before using each archive. Network access is needed on first
configure; CMake caches the extracted sources in the build directory.
`dependencies.json` records source URLs, versions and checksums. The adjacent
license files are the upstream licenses for these dependencies.

- doctest 2.5.3: MIT license, `doctest-LICENSE.txt`.
- Google Benchmark 1.9.5: Apache-2.0 license, `benchmark-LICENSE.txt`.
