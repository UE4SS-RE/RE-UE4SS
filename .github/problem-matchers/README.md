# Problem Matchers

[GitHub Official Problem Matchers Documentation](https://github.com/actions/toolkit/blob/main/docs/problem-matchers.md)

## Summary

Problem Matchers are used to scan the output of GitHub workflows and display any errors/warnings/information directly on the Pull Request web view.

### Purpose

- **Identify Issues:** Detects and highlights errors, warnings, and informational messages from action outputs.
- **Create Annotations:** Automatically generates GitHub Annotations and log file decorations when matches are found.
- **Improve Visibility:** Surfaces critical information prominently in the UI for easier debugging and review.

### Use Cases

- **Automated Code Review:** Integrate with tools to automatically detect coding standard violations.
- **Continuous Integration:** Enhance CI workflows by providing immediate feedback on build or test failures.
- **Custom Log Parsing:** Create custom matchers to identify and annotate specific patterns in log outputs.

### Key Features

- **Regex-Based Matching:** Uses regular expressions to define the patterns to look for in the logs.
- **Customization:** Allows users to define custom matchers and register/unregister them in GitHub workflows.
