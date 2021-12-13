* It should be possible to build and test using the same or very similar commands on Linux or MacOS, and on CircleCI.
* Checkout `../.circleci/config.yml` for the commands themselves.
* Building and testing is going through a refactor, and for a list of not yet implemented features try `make check_todo`.
* Todo: Implement integration testing: Figure out how to use speculos emulator & binary app (instead of the physical device) from the production Flow Port web site, and automate them both (e.g. with something like Selenium browser automation) for integration tests.
