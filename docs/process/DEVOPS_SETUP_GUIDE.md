# ClimbingGame DevOps and Development Environment Setup Guide

## Overview

This guide provides comprehensive instructions for setting up the development infrastructure and CI/CD pipeline for ClimbingGame, a physics-intensive multiplayer climbing game built with Unreal Engine 5.6.

## Table of Contents

1. [System Requirements](#system-requirements)
2. [Development Environment Setup](#development-environment-setup)
3. [Version Control Configuration](#version-control-configuration)
4. [CI/CD Pipeline Setup](#cicd-pipeline-setup)
5. [Build Automation](#build-automation)
6. [Testing Infrastructure](#testing-infrastructure)
7. [Performance Monitoring](#performance-monitoring)
8. [Asset Pipeline](#asset-pipeline)
9. [Deployment Configuration](#deployment-configuration)
10. [Troubleshooting](#troubleshooting)

## System Requirements

### Minimum Hardware Requirements
- **CPU**: 8-core Intel i7 or AMD Ryzen 7 (3.0+ GHz)
- **RAM**: 32GB DDR4 (64GB recommended for large asset workflows)
- **GPU**: NVIDIA GTX 1080 / RTX 3060 or AMD equivalent (8GB VRAM minimum)
- **Storage**: 500GB SSD (NVMe recommended)
- **Network**: Gigabit Ethernet for large asset synchronization

### Software Requirements
- **OS**: Windows 10/11 (64-bit), Ubuntu 20.04+, or macOS 11+
- **Unreal Engine**: 5.6 (exact version required)
- **Visual Studio**: 2022 (Windows) or Xcode 13+ (macOS)
- **Git**: 2.30+ with Git LFS support
- **Python**: 3.9+ (for automation scripts)
- **Node.js**: 16+ (for web-based tools)

## Development Environment Setup

### 1. Unreal Engine 5.6 Installation

#### Windows
```bash
# Download Epic Games Launcher
# Install Unreal Engine 5.6 through the launcher
# Or use pre-built binaries for CI/CD environments

# Set environment variables
set UE_PATH=C:\Program Files\Epic Games\UE_5.6
set PATH=%UE_PATH%\Engine\Binaries\Win64;%PATH%
```

#### Linux
```bash
# Clone Unreal Engine from source (requires Epic Games account linking)
git clone https://github.com/EpicGames/UnrealEngine.git -b 5.6
cd UnrealEngine

# Setup and compile
./Setup.sh
./GenerateProjectFiles.sh
make

# Set environment variables
export UE_PATH=/opt/UnrealEngine
export PATH=$UE_PATH/Engine/Binaries/Linux:$PATH
```

#### macOS
```bash
# Install Xcode and command line tools
xcode-select --install

# Download Unreal Engine through Epic Games Launcher
# Or compile from source similar to Linux

export UE_PATH=/Applications/UE_5.6
export PATH=$UE_PATH/Engine/Binaries/Mac:$PATH
```

### 2. IDE and Tools Setup

#### Visual Studio Code Configuration
Create `.vscode/settings.json`:
```json
{
    "C_Cpp.default.compilerPath": "${env:UE_PATH}/Engine/Extras/ThirdPartyNotUE/SDKs/HostWin64/Win64/bin/clang++.exe",
    "C_Cpp.default.includePath": [
        "${workspaceFolder}/Source/**",
        "${env:UE_PATH}/Engine/Source/**"
    ],
    "C_Cpp.default.defines": [
        "UE_BUILD_DEVELOPMENT=1",
        "WITH_ENGINE=1",
        "WITH_UNREAL_DEVELOPER_TOOLS=1"
    ],
    "files.associations": {
        "*.uproject": "json",
        "*.uplugin": "json"
    }
}
```

#### Visual Studio Configuration (Windows)
- Install Unreal Engine integration
- Configure IntelliSense for Unreal Engine headers
- Set up debugging for attached processes

### 3. Python Environment Setup
```bash
# Create virtual environment for automation scripts
python -m venv venv_climbinggame
source venv_climbinggame/bin/activate  # Linux/macOS
# or
venv_climbinggame\Scripts\activate  # Windows

# Install required packages
pip install -r Scripts/requirements.txt
```

Create `Scripts/requirements.txt`:
```txt
psutil>=5.8.0
Pillow>=8.3.0
matplotlib>=3.5.0
PyYAML>=6.0
requests>=2.26.0
pytest>=6.2.0
pytest-asyncio>=0.18.0
websockets>=10.0
numpy>=1.21.0
pandas>=1.3.0
```

## Version Control Configuration

### 1. Git Repository Setup
```bash
# Initialize repository
git init
git lfs install

# Add remotes (replace with your repository URLs)
git remote add origin https://github.com/YourOrg/ClimbingGame.git
git remote add upstream https://github.com/YourOrg/ClimbingGame-upstream.git
```

### 2. Git LFS Configuration
The `.gitattributes` file is already configured for Unreal Engine assets. Verify tracking:
```bash
git lfs track
git lfs ls-files
```

### 3. Branch Strategy
```bash
# Main development branches
git checkout -b develop origin/develop
git checkout -b feature/your-feature-name develop

# Release branches
git checkout -b release/v1.0.0 develop

# Hotfix branches
git checkout -b hotfix/critical-fix main
```

### 4. Pre-commit Hooks Setup
Install pre-commit hooks:
```bash
pip install pre-commit
pre-commit install
```

Create `.pre-commit-config.yaml`:
```yaml
repos:
-   repo: https://github.com/pre-commit/pre-commit-hooks
    rev: v4.4.0
    hooks:
    -   id: trailing-whitespace
    -   id: end-of-file-fixer
    -   id: check-yaml
    -   id: check-json
-   repo: local
    hooks:
    -   id: asset-validation
        name: Validate Assets
        entry: python Scripts/CI/validate_assets.py
        language: python
        pass_filenames: false
        always_run: true
```

## CI/CD Pipeline Setup

### 1. GitHub Actions Configuration

The CI/CD pipeline is defined in `.github/workflows/ci-cd-pipeline.yml`. To set up:

1. **Configure Repository Secrets**:
   ```
   TEAMS_WEBHOOK: Microsoft Teams webhook URL
   AWS_ACCESS_KEY_ID: AWS credentials for artifact storage
   AWS_SECRET_ACCESS_KEY: AWS secret key
   UE_LICENSE_KEY: Unreal Engine license key
   ```

2. **Self-hosted Runners** (Recommended for UE builds):
   ```bash
   # Download and configure GitHub Actions runner
   mkdir actions-runner && cd actions-runner
   curl -o actions-runner-win-x64-2.300.2.zip -L https://github.com/actions/runner/releases/download/v2.300.2/actions-runner-win-x64-2.300.2.zip
   tar xzf ./actions-runner-win-x64-2.300.2.zip
   
   # Configure
   ./config.cmd --url https://github.com/YourOrg/ClimbingGame --token YOUR_TOKEN
   
   # Install as service
   ./svc.sh install
   ./svc.sh start
   ```

### 2. Jenkins Alternative Setup

Create `Jenkinsfile`:
```groovy
pipeline {
    agent any
    
    environment {
        UE_PATH = 'C:\\UnrealEngine\\UE_5.6'
        PROJECT_NAME = 'ClimbingGame'
    }
    
    stages {
        stage('Checkout') {
            steps {
                checkout scm
                script {
                    sh 'git lfs pull'
                }
            }
        }
        
        stage('Asset Validation') {
            steps {
                script {
                    sh 'python Scripts/CI/validate_assets.py --project-dir . --report-file asset-validation.json'
                }
            }
        }
        
        stage('Build') {
            parallel {
                stage('Windows Build') {
                    steps {
                        script {
                            bat '''
                                python Scripts/Build/build_project.py ^
                                    --engine-path "%UE_PATH%" ^
                                    --platforms Win64 ^
                                    --configurations Development Shipping
                            '''
                        }
                    }
                }
                stage('Linux Build') {
                    steps {
                        script {
                            sh '''
                                python Scripts/Build/build_project.py \
                                    --engine-path "$UE_PATH" \
                                    --platforms Linux \
                                    --configurations Development
                            '''
                        }
                    }
                }
            }
        }
        
        stage('Testing') {
            steps {
                script {
                    bat '''
                        python Scripts/CI/run_multiplayer_tests.py ^
                            --build-path ./Binaries ^
                            --test-scenarios basic_connection cooperative_climbing
                    '''
                }
            }
        }
        
        stage('Performance Benchmarking') {
            when {
                anyOf {
                    branch 'main'
                    branch 'develop'
                }
            }
            steps {
                script {
                    bat '''
                        python Scripts/CI/performance_benchmark.py ^
                            --build-path ./Binaries ^
                            --scenarios climbing_physics rope_simulation
                    '''
                }
            }
        }
    }
    
    post {
        always {
            archiveArtifacts artifacts: 'TestResults/**/*', allowEmptyArchive: true
            publishHTML([
                allowMissing: false,
                alwaysLinkToLastBuild: true,
                keepAll: true,
                reportDir: 'TestResults',
                reportFiles: '*.html',
                reportName: 'Test Results'
            ])
        }
    }
}
```

## Build Automation

### 1. Local Build Scripts

#### Windows Build Script (`Scripts/build_windows.bat`):
```batch
@echo off
echo Building ClimbingGame for Windows...

set UE_PATH=C:\Program Files\Epic Games\UE_5.6
set PROJECT_PATH=%CD%\ClimbingGame.uproject

REM Generate project files
"%UE_PATH%\Engine\Binaries\DotNET\UnrealBuildTool.exe" -projectfiles -project="%PROJECT_PATH%" -game -rocket -progress

REM Build Development
"%UE_PATH%\Engine\Binaries\DotNET\UnrealBuildTool.exe" ClimbingGame Win64 Development -Project="%PROJECT_PATH%" -WaitMutex

REM Package Shipping
"%UE_PATH%\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="%PROJECT_PATH%" -platform=Win64 -configuration=Shipping -cook -build -stage -package -archive -archivedirectory="%CD%\Packaged"

echo Build completed!
pause
```

#### Linux Build Script (`Scripts/build_linux.sh`):
```bash
#!/bin/bash
echo "Building ClimbingGame for Linux..."

UE_PATH="/opt/UnrealEngine"
PROJECT_PATH="$(pwd)/ClimbingGame.uproject"

# Generate project files
"$UE_PATH/Engine/Binaries/DotNET/UnrealBuildTool" -projectfiles -project="$PROJECT_PATH" -game -rocket -progress

# Build Development
"$UE_PATH/Engine/Binaries/DotNET/UnrealBuildTool" ClimbingGame Linux Development -Project="$PROJECT_PATH"

# Package (if needed)
"$UE_PATH/Engine/Build/BatchFiles/Linux/RunUAT.sh" BuildCookRun -project="$PROJECT_PATH" -platform=Linux -configuration=Shipping -cook -build -stage -package -archive -archivedirectory="$(pwd)/Packaged"

echo "Build completed!"
```

### 2. Automated Build Triggers

Configure webhook triggers:
```bash
# GitHub webhook example
curl -X POST \
  -H "Accept: application/vnd.github.v3+json" \
  -H "Authorization: token YOUR_TOKEN" \
  https://api.github.com/repos/YourOrg/ClimbingGame/hooks \
  -d '{
    "name": "web",
    "active": true,
    "events": ["push", "pull_request"],
    "config": {
      "url": "http://your-ci-server.com/webhook",
      "content_type": "json"
    }
  }'
```

## Testing Infrastructure

### 1. Unit Testing Setup

Create test configuration in `Config/DefaultEngine.ini`:
```ini
[/Script/UnrealEd.AutomationTestSettings]
bTreatLogWarningsAsTestErrors=True
bTreatLogErrorsAsTestErrors=True
```

### 2. Multiplayer Test Environment

Set up dedicated test servers:
```bash
# Launch test server
ClimbingGame.exe TestLevel -server -port=7777 -log -unattended

# Connect test clients
ClimbingGame.exe 127.0.0.1:7777 -log -unattended -nullrhi
```

### 3. Performance Test Automation

Configure performance baselines in `Scripts/CI/performance_baselines.json`:
```json
{
  "climbing_physics": {
    "min_fps": 60,
    "max_memory_mb": 2000,
    "max_cpu_percent": 80
  },
  "rope_simulation": {
    "min_fps": 45,
    "max_memory_mb": 1500,
    "max_cpu_percent": 85
  }
}
```

## Performance Monitoring

### 1. Real-time Monitoring Setup

Install monitoring tools:
```bash
# Install Grafana and Prometheus
docker run -d --name=grafana -p 3000:3000 grafana/grafana
docker run -d --name=prometheus -p 9090:9090 prom/prometheus
```

Configure monitoring dashboard for:
- Build success/failure rates
- Test execution times
- Performance benchmark trends
- Asset validation results
- Memory usage patterns

### 2. Performance Regression Detection

Set up automated regression detection:
```python
# Example performance regression check
def check_performance_regression(current_results, baseline_results, threshold=0.1):
    for metric, current_value in current_results.items():
        baseline_value = baseline_results.get(metric)
        if baseline_value:
            change_ratio = (current_value - baseline_value) / baseline_value
            if change_ratio > threshold:
                return False, f"Performance regression in {metric}: {change_ratio:.2%}"
    return True, "No significant regression detected"
```

## Asset Pipeline

### 1. Asset Validation Rules

Configure validation rules in `Scripts/CI/asset_validation_config.yaml`:
```yaml
textures:
  max_resolution: [4096, 4096]
  supported_formats: ['.png', '.jpg', '.tga']
  power_of_two_required: true
  max_file_size_mb: 50

meshes:
  max_polygons: 100000
  supported_formats: ['.fbx', '.obj']
  max_file_size_mb: 100

audio:
  supported_formats: ['.wav', '.ogg']
  max_file_size_mb: 25
  sample_rates: [44100, 48000]
```

### 2. Asset Optimization Pipeline

Set up automated asset optimization:
```bash
# Texture optimization
for file in Content/Textures/**/*.png; do
    # Compress textures
    pngquant --quality=80-95 --ext .png --force "$file"
done

# Mesh optimization
# Use Unreal Engine's built-in optimization tools
```

## Deployment Configuration

### 1. Environment Configuration

Create environment-specific configs:

**Development** (`Config/Development/DefaultEngine.ini`):
```ini
[/Script/Engine.Engine]
bUseFixedFrameRate=False
FixedFrameRate=60.000000

[/Script/Engine.RendererSettings]
r.DefaultFeature.AntiAliasing=2
```

**Staging** (`Config/Staging/DefaultEngine.ini`):
```ini
[/Script/Engine.Engine]
bUseFixedFrameRate=True
FixedFrameRate=60.000000

[ConsoleVariables]
r.SetRes=1920x1080f
```

**Production** (`Config/Shipping/DefaultEngine.ini`):
```ini
[/Script/Engine.Engine]
bUseFixedFrameRate=True
FixedFrameRate=60.000000

[/Script/Engine.RendererSettings]
r.DefaultFeature.AntiAliasing=3
```

### 2. Deployment Scripts

Create deployment automation in `Scripts/Deploy/deploy.py`:
```python
#!/usr/bin/env python3
import os
import sys
import shutil
from pathlib import Path

def deploy_to_environment(build_path, environment, config):
    """Deploy build to specified environment"""
    
    # Copy build files
    target_path = config['environments'][environment]['path']
    shutil.copytree(build_path, target_path, dirs_exist_ok=True)
    
    # Update configuration
    config_source = f"Config/{environment.title()}"
    config_target = f"{target_path}/Config"
    shutil.copytree(config_source, config_target, dirs_exist_ok=True)
    
    # Run post-deployment tests
    run_smoke_tests(target_path, environment)
    
    print(f"Deployment to {environment} completed successfully")

if __name__ == "__main__":
    # Deployment logic here
    pass
```

## Troubleshooting

### Common Issues and Solutions

#### 1. Build Failures

**Issue**: "Failed to generate project files"
**Solution**:
```bash
# Clean generated files
rm -rf Binaries/ Intermediate/ .vs/
# Regenerate
UnrealBuildTool -projectfiles -project="ClimbingGame.uproject" -game -rocket -progress
```

**Issue**: "Out of memory during compilation"
**Solution**:
```bash
# Reduce parallel compilation
UnrealBuildTool ClimbingGame Win64 Development -MaxParallelActions=4
```

#### 2. Git LFS Issues

**Issue**: "Git LFS quota exceeded"
**Solution**:
```bash
# Check LFS usage
git lfs fsck

# Clean old LFS objects
git lfs prune --verify-remote
```

#### 3. Performance Test Failures

**Issue**: "Inconsistent performance results"
**Solution**:
- Ensure dedicated test hardware
- Run tests during low system load
- Increase warmup time for consistent results

#### 4. Asset Validation Failures

**Issue**: "Texture validation fails"
**Solution**:
```bash
# Check texture properties
python Scripts/CI/validate_assets.py --project-dir . --verbose

# Fix common issues
find Content -name "*.png" -exec optipng {} \;
```

### Support and Resources

- **Unreal Engine Documentation**: https://docs.unrealengine.com/
- **CI/CD Best Practices**: Internal wiki or documentation
- **Performance Optimization Guide**: Team-specific guidelines
- **Asset Creation Standards**: Art team documentation

### Contact Information

- **DevOps Team**: devops@yourcompany.com
- **Build Issues**: Slack #build-support
- **Emergency**: On-call rotation schedule

---

## Quick Start Checklist

- [ ] Install Unreal Engine 5.6
- [ ] Clone repository with LFS
- [ ] Set up development environment
- [ ] Configure IDE
- [ ] Run first local build
- [ ] Execute asset validation
- [ ] Run sample tests
- [ ] Configure CI/CD webhook
- [ ] Deploy to staging environment
- [ ] Set up performance monitoring

This setup ensures a robust development infrastructure capable of handling the complex requirements of ClimbingGame's physics-intensive, multiplayer architecture while maintaining high code quality and performance standards.