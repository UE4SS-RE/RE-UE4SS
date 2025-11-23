# Contributing to RE-UE4SS

Thank you for taking the time to contribute!

All types of contributions are encouraged and valued. This guide outlines the process and guidelines for contributing to the project.

## Table of Contents

- [Reporting issues](#reporting-issues)
- [Pull Request Guidelines](#pull-request-guidelines)
- [Development Workflow](#development-workflow)
- [Code Style Guidelines](#code-style-guidelines)
- [Commit Message Guidelines](#commit-message-guidelines)
- [Upgrade Guidelines](#upgrade-guidelines)
- [License](#license)

## Reporting Issues

Before you ask a question, it is best to search for existing [issues](https://github.com/UE4SS-RE/RE-UE4SS/issues) and read the available [documentation](https://docs.ue4ss.com/).

If you still need clarification:
- Open an [issue](https://github.com/UE4SS-RE/RE-UE4SS/issues) using the correct template
- Provide as much context as you can about what you're experiencing
- Provide project and platform versions (OS, etc.)
- If the issue is build related, provide toolchain information (compiler version, etc.) 
- Provide UE4SS.log whenever it's available, even if it doesn't seem useful

> **Security Issues**: Never report security-related issues, vulnerabilities or bugs including sensitive information to the issue tracker. Instead, please email sensitive bugs to <security@ue4ss.com>.

## Pull Request Guidelines

### General Requirements

1. Before submitting a PR, please ensure your changes have been tested on:
   - At least one Unreal Engine version between 4.11 and 4.25 (the version where UProperty was replaced by FProperty)
   - At least one version after 4.25, preferably UE 5.1 or newer

2. All PRs must include:
   - A clear description of the changes
   - Steps to reproduce the issue being fixed (if applicable)
   - Code to reproduce the problem it fixes or feature it adds
   - Any relevant test cases or examples
   - Updates to the [Changelog.md](/assets/Changelog.md) file with relevant changes

3. Code should be well-documented and follow the existing style conventions of the repository.

## Development Workflow

1. Fork the repository
2. Create a new branch for your feature or bug fix
3. Implement your changes
4. Run the appropriate tests for your changes
5. Update the documentation if necessary
6. Submit a pull request

## Code Style Guidelines

### UEPseudo Code

- Code in UEPseudo should match Unreal Engine's formatting and style conventions
- When implementing code directly from Unreal Engine, maintain the original style
- Any deviations or fixes from the original UE code should be enclosed between these comments:
  ```cpp
  // RE-UE4SS FIX (Contributor initials or username): [explanation of why the fix is needed]
  // Modified code goes here
  // RE-UE4SS FIX END
  ```


### Main Repository Code

- All code in the main repository should have `clang-format` run against it before submission
- Follow the existing code style of the repository for consistency
- Use meaningful variable and function names
- Include comments for complex logic or non-obvious implementations

## Commit Message Guidelines

- We favor [Conventional Commits](https://www.conventionalcommits.org/) format for all commit messages.
  ```
  <type>[optional scope]: <description>
  
  [optional body]
  
  [optional footer(s)]
  ```
- Common types include: feat, fix, docs, style, refactor, test, chore
- Reference issue numbers in your commit messages when applicable
- Keep commits focused on a single logical change

## Upgrade Guidelines

We maintain an upgrade guide to track breaking changes and help users migrate between versions.

### For Contributors

When making breaking changes to the API:

1. You must document all breaking changes in `/docs/upgrade-guide.md`, including:
   - A clear description of what changed
   - The reason for the change
   - Code examples showing the old way vs. the new way
   - Any special migration steps

2. Breaking changes should follow these principles:
   - Minimize the impact on existing code
   - Provide deprecation warnings where possible before removal
   - Consider backward compatibility or transitional APIs
   - Clearly communicate the change through version numbering (follow semver)

3. When submitting a PR with breaking changes:
   - Explicitly tag it with `breaking-change` in the PR title
   - Update the Changelog with a "Breaking Changes" subsection
   - Reference the PR in the upgrade guide

### For API Users

- The `/docs/upgrade-guide.md` file contains version-by-version migration instructions
- Each major and minor version has its own section detailing changes
- Always review the upgrade guide before updating to a new version
- The guide includes code examples for adapting to API changes

The upgrade guide is organized chronologically with the most recent version at the top, making it easy to follow incremental upgrades.

## License

### RE-UE4SS License
By contributing to RE-UE4SS, you agree that your contributions will be licensed under the [MIT licence](https://github.com/UE4SS-RE/RE-UE4SS/blob/main/LICENSE)

### UEPseudo Code Licensing
UEPseudo code is subject to Epic Games' licensing terms.

Thank you for helping improve RE-UE4SS!

## Attribution
This guide is based on the [contributing.md](https://contributing.md/generator)!