# railway-ticketing-system
A railway ticket reservation system with booking and cancellation features.

Product Owner - Aaryaprasad

Developer - Deeptanshu Kumar

Testing - Amar

## Tech stack
- **Backend:** C++17 REST API ([cpp-httplib](https://github.com/yhirose/cpp-httplib), [nlohmann/json](https://github.com/nlohmann/json)), SQLite
- **Security:** PBKDF2-HMAC-SHA256 password hashing and random session tokens (OpenSSL), parameterised SQL only
- **Frontend:** static HTML/CSS/JavaScript served by the backend (`web/`)
- **Tests:** GoogleTest, gcov/gcovr coverage (line + branch)
- **CI/CD:** GitHub Actions (build, test, coverage, cppcheck, CodeQL, Docker build and GHCR publish)

## Build and run
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/rtrs_server          # http://localhost:8080
```
Environment variables: `RTRS_PORT` (default 8080), `RTRS_DB` (default `rtrs.db`), `RTRS_WEB_DIR` (default `web`).

Docker: `docker build -t rtrs . && docker run -p 8080:8080 rtrs`

## Workflow
1. Developer works on one feature branch per Jira story (e.g. `RTRS-9-user-registration`), one commit per story.
2. Pull request into `test`; CI must be green.
3. Tester verifies on `test`, then merges `test` into `main`.
4. Merge to `main` publishes the Docker image (CD).

Commit messages start with the Jira key, e.g. `RTRS-9: Add user registration`.
