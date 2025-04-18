# Contributing to the Repository

## Overview

### 1. Clone the Repository
Clone the repository to your local machine and create a new branch for your work:

```bash
git clone https://github.com/your-org/repo-name.git
cd repo-name
git checkout -b feature/your-feature-name
```

Branch Naming: Use descriptive branch names based on the feature or fix you're working on.

### 2. Make Changes and Commit
Make your changes, and commit them with a descriptive message:

```bash
git add .
git commit -m "feat: Add Kalman filter for multi-target tracking"
```

### 3. Push the Branch
Push your branch to the repository:

```bash
git push origin feature/your-feature-name
```

### 4. Create a Pull Request
* Go to the repository on GitHub.
* Click on the "Pull Requests" tab and select "New Pull Request."
* Set the base branch (e.g., main) and compare it with your branch (e.g., feature/your-feature-name).
* Add a clear description of the changes in the pull request.

### 5. Delete the Branch After Merging
Once your pull request is reviewed, approved, and merged, delete your branch to keep the repository clean:

```bash
git branch -d feature/your-feature-name    # Delete locally
git push origin --delete feature/your-feature-name  # Delete remotely
```

## Code Formatting
To maintain a consistent code style across the project, we use Clang-Format. All contributors must ensure their code adheres to the formatting rules specified in the .clang-format file located in the project root.
