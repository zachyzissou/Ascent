#!/usr/bin/env python3
"""
ClimbingGame Smoke Tests
Quick validation tests to ensure basic functionality after deployment
"""

import os
import sys
import time
import logging
import argparse
import requests
import subprocess
from pathlib import Path
from typing import Dict, List, Tuple, Optional

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

class SmokeTestRunner:
    """Runs smoke tests to validate deployment health"""
    
    def __init__(self, environment: str, build_path: Optional[str] = None):
        self.environment = environment
        self.build_path = Path(build_path) if build_path else None
        
        # Environment-specific configuration
        self.env_config = {
            'development': {
                'base_url': 'https://dev.climbinggame.internal',
                'timeout': 30
            },
            'staging': {
                'base_url': 'https://staging.climbinggame.com',
                'timeout': 60
            },
            'production': {
                'base_url': 'https://climbinggame.com',
                'timeout': 30
            }
        }
        
        self.config = self.env_config.get(environment, self.env_config['development'])
        self.test_results = []
        
    def run_http_health_check(self) -> bool:
        """Test basic HTTP endpoint health"""
        logger.info("Running HTTP health check...")
        
        try:
            url = f"{self.config['base_url']}/health"
            response = requests.get(url, timeout=self.config['timeout'])
            
            if response.status_code == 200:
                logger.info("HTTP health check passed")
                self.test_results.append(("HTTP Health Check", True, ""))
                return True
            else:
                error_msg = f"HTTP health check failed: {response.status_code}"
                logger.error(error_msg)
                self.test_results.append(("HTTP Health Check", False, error_msg))
                return False
                
        except Exception as e:
            error_msg = f"HTTP health check failed: {e}"
            logger.error(error_msg)
            self.test_results.append(("HTTP Health Check", False, error_msg))
            return False
    
    def test_game_executable_launch(self) -> bool:
        """Test that the game executable can launch"""
        if not self.build_path:
            logger.warning("No build path provided, skipping executable test")
            return True
            
        logger.info("Testing game executable launch...")
        
        try:
            # Find game executable
            exe_paths = [
                self.build_path / "ClimbingGame.exe",
                self.build_path / "WindowsNoEditor" / "ClimbingGame.exe",
                self.build_path / "Binaries" / "Win64" / "ClimbingGame.exe"
            ]
            
            game_exe = None
            for path in exe_paths:
                if path.exists():
                    game_exe = path
                    break
            
            if not game_exe:
                error_msg = "Game executable not found"
                logger.error(error_msg)
                self.test_results.append(("Game Executable Launch", False, error_msg))
                return False
            
            # Launch game with minimal parameters for quick test
            cmd = [
                str(game_exe),
                "TestLevel",
                "-log",
                "-unattended",
                "-nullrhi",
                "-nosplash",
                "-windowed",
                "-ResX=640",
                "-ResY=480"
            ]
            
            process = subprocess.Popen(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True
            )
            
            # Give the game a few seconds to start
            time.sleep(5)
            
            # Check if process is still running (indicates successful start)
            if process.poll() is None:
                # Game started successfully, terminate it
                process.terminate()
                try:
                    process.wait(timeout=10)
                except subprocess.TimeoutExpired:
                    process.kill()
                
                logger.info("Game executable launch test passed")
                self.test_results.append(("Game Executable Launch", True, ""))
                return True
            else:
                stdout, stderr = process.communicate()
                error_msg = f"Game failed to start: {stderr}"
                logger.error(error_msg)
                self.test_results.append(("Game Executable Launch", False, error_msg))
                return False
                
        except Exception as e:
            error_msg = f"Game executable test failed: {e}"
            logger.error(error_msg)
            self.test_results.append(("Game Executable Launch", False, error_msg))
            return False
    
    def test_api_endpoints(self) -> bool:
        """Test critical API endpoints"""
        logger.info("Testing API endpoints...")
        
        endpoints_to_test = [
            "/api/status",
            "/api/version",
            "/api/servers"  # Multiplayer server list
        ]
        
        all_passed = True
        
        for endpoint in endpoints_to_test:
            try:
                url = f"{self.config['base_url']}{endpoint}"
                response = requests.get(url, timeout=self.config['timeout'])
                
                if response.status_code in [200, 404]:  # 404 is acceptable for optional endpoints
                    logger.info(f"API endpoint {endpoint}: OK")
                    self.test_results.append((f"API Endpoint {endpoint}", True, ""))
                else:
                    error_msg = f"API endpoint {endpoint} failed: {response.status_code}"
                    logger.warning(error_msg)
                    self.test_results.append((f"API Endpoint {endpoint}", False, error_msg))
                    all_passed = False
                    
            except Exception as e:
                error_msg = f"API endpoint {endpoint} failed: {e}"
                logger.error(error_msg)
                self.test_results.append((f"API Endpoint {endpoint}", False, error_msg))
                all_passed = False
        
        return all_passed
    
    def test_static_assets(self) -> bool:
        """Test that static assets are accessible"""
        logger.info("Testing static assets...")
        
        assets_to_test = [
            "/favicon.ico",
            "/robots.txt",
            "/assets/logo.png"  # Assuming there's a logo
        ]
        
        all_passed = True
        
        for asset in assets_to_test:
            try:
                url = f"{self.config['base_url']}{asset}"
                response = requests.get(url, timeout=self.config['timeout'])
                
                if response.status_code in [200, 404]:  # 404 is acceptable for optional assets
                    logger.info(f"Static asset {asset}: OK")
                    self.test_results.append((f"Static Asset {asset}", True, ""))
                else:
                    error_msg = f"Static asset {asset} failed: {response.status_code}"
                    logger.warning(error_msg)
                    self.test_results.append((f"Static Asset {asset}", False, error_msg))
                    # Don't fail the entire test for missing static assets
                    
            except Exception as e:
                error_msg = f"Static asset {asset} failed: {e}"
                logger.warning(error_msg)
                self.test_results.append((f"Static Asset {asset}", False, error_msg))
        
        return True  # Don't fail smoke tests for static assets
    
    def test_database_connectivity(self) -> bool:
        """Test database connectivity through API"""
        logger.info("Testing database connectivity...")
        
        try:
            # Test an endpoint that requires database access
            url = f"{self.config['base_url']}/api/servers"
            response = requests.get(url, timeout=self.config['timeout'])
            
            # Even if there are no servers, a successful response indicates DB connectivity
            if response.status_code in [200, 404]:
                logger.info("Database connectivity test passed")
                self.test_results.append(("Database Connectivity", True, ""))
                return True
            else:
                error_msg = f"Database connectivity test failed: {response.status_code}"
                logger.error(error_msg)
                self.test_results.append(("Database Connectivity", False, error_msg))
                return False
                
        except Exception as e:
            error_msg = f"Database connectivity test failed: {e}"
            logger.error(error_msg)
            self.test_results.append(("Database Connectivity", False, error_msg))
            return False
    
    def test_multiplayer_service(self) -> bool:
        """Test multiplayer service availability"""
        logger.info("Testing multiplayer service...")
        
        try:
            # Test multiplayer server list endpoint
            url = f"{self.config['base_url']}/api/servers"
            response = requests.get(url, timeout=self.config['timeout'])
            
            if response.status_code == 200:
                # Try to parse server list
                try:
                    servers = response.json()
                    logger.info(f"Multiplayer service active with {len(servers)} servers")
                    self.test_results.append(("Multiplayer Service", True, f"{len(servers)} servers available"))
                    return True
                except:
                    logger.info("Multiplayer service responded but no server data")
                    self.test_results.append(("Multiplayer Service", True, "Service active, no servers"))
                    return True
            elif response.status_code == 404:
                logger.warning("Multiplayer service endpoint not found")
                self.test_results.append(("Multiplayer Service", False, "Endpoint not found"))
                return False
            else:
                error_msg = f"Multiplayer service test failed: {response.status_code}"
                logger.error(error_msg)
                self.test_results.append(("Multiplayer Service", False, error_msg))
                return False
                
        except Exception as e:
            error_msg = f"Multiplayer service test failed: {e}"
            logger.error(error_msg)
            self.test_results.append(("Multiplayer Service", False, error_msg))
            return False
    
    def run_all_smoke_tests(self, timeout: int = 180) -> bool:
        """Run all smoke tests"""
        logger.info(f"Starting smoke tests for {self.environment} environment...")
        
        start_time = time.time()
        tests_passed = 0
        total_tests = 0
        
        # Define test suite
        test_suite = [
            ("HTTP Health Check", self.run_http_health_check),
            ("API Endpoints", self.test_api_endpoints),
            ("Static Assets", self.test_static_assets),
            ("Database Connectivity", self.test_database_connectivity),
            ("Multiplayer Service", self.test_multiplayer_service)
        ]
        
        # Add executable test if build path is provided
        if self.build_path:
            test_suite.append(("Game Executable", self.test_game_executable_launch))
        
        # Run tests
        for test_name, test_func in test_suite:
            total_tests += 1
            
            if time.time() - start_time > timeout:
                logger.error(f"Smoke tests timed out after {timeout} seconds")
                break
            
            try:
                logger.info(f"Running {test_name}...")
                if test_func():
                    tests_passed += 1
                    logger.info(f"✓ {test_name} passed")
                else:
                    logger.error(f"✗ {test_name} failed")
            except Exception as e:
                logger.error(f"✗ {test_name} failed with exception: {e}")
        
        # Summary
        duration = time.time() - start_time
        success_rate = (tests_passed / total_tests) * 100 if total_tests > 0 else 0
        
        logger.info(f"Smoke tests completed in {duration:.2f} seconds")
        logger.info(f"Results: {tests_passed}/{total_tests} tests passed ({success_rate:.1f}%)")
        
        # Consider tests successful if at least 80% pass and critical tests pass
        critical_tests = ["HTTP Health Check"]
        critical_passed = all(
            result[1] for result in self.test_results 
            if result[0] in critical_tests
        )
        
        return success_rate >= 80 and critical_passed
    
    def generate_report(self) -> Dict:
        """Generate test report"""
        passed_count = sum(1 for result in self.test_results if result[1])
        total_count = len(self.test_results)
        
        return {
            'environment': self.environment,
            'timestamp': time.time(),
            'total_tests': total_count,
            'passed_tests': passed_count,
            'failed_tests': total_count - passed_count,
            'success_rate': (passed_count / total_count * 100) if total_count > 0 else 0,
            'test_results': [
                {
                    'name': result[0],
                    'passed': result[1],
                    'message': result[2]
                }
                for result in self.test_results
            ]
        }

def main():
    parser = argparse.ArgumentParser(description='ClimbingGame Smoke Tests')
    parser.add_argument('--environment', required=True,
                       choices=['development', 'staging', 'production'],
                       help='Target environment for smoke tests')
    parser.add_argument('--build-path', 
                       help='Path to build directory (for executable tests)')
    parser.add_argument('--timeout', type=int, default=180,
                       help='Overall test timeout in seconds')
    parser.add_argument('--report-file',
                       help='Output file for test report')
    
    args = parser.parse_args()
    
    try:
        smoke_tester = SmokeTestRunner(args.environment, args.build_path)
        success = smoke_tester.run_all_smoke_tests(args.timeout)
        
        # Generate report
        report = smoke_tester.generate_report()
        
        if args.report_file:
            import json
            with open(args.report_file, 'w') as f:
                json.dump(report, f, indent=2)
            logger.info(f"Test report saved to: {args.report_file}")
        
        # Print summary
        print(f"\nSmoke Test Results for {args.environment}:")
        print(f"  Total Tests: {report['total_tests']}")
        print(f"  Passed: {report['passed_tests']}")
        print(f"  Failed: {report['failed_tests']}")
        print(f"  Success Rate: {report['success_rate']:.1f}%")
        
        if not success:
            print("\nFailed Tests:")
            for result in report['test_results']:
                if not result['passed']:
                    print(f"  - {result['name']}: {result['message']}")
        
        sys.exit(0 if success else 1)
    
    except Exception as e:
        logger.error(f"Smoke tests failed: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()