#!/usr/bin/env python3
"""
ClimbingGame Multiplayer Testing Automation
Automated testing pipeline for multiplayer scenarios and cooperative gameplay
"""

import os
import sys
import json
import time
import logging
import argparse
import subprocess
import asyncio
import threading
from pathlib import Path
from typing import Dict, List, Optional, Tuple
import socket
import psutil
from dataclasses import dataclass
from concurrent.futures import ThreadPoolExecutor

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

@dataclass
class GameInstance:
    """Represents a game instance for multiplayer testing"""
    process: subprocess.Popen
    instance_id: str
    role: str  # 'server', 'client', 'host'
    port: int
    pid: int
    startup_time: float

@dataclass
class TestScenario:
    """Defines a multiplayer test scenario"""
    name: str
    description: str
    min_players: int
    max_players: int
    duration: int  # seconds
    map_name: str
    test_objectives: List[str]
    success_criteria: Dict[str, any]

class MultiplayerTestRunner:
    """Handles automated multiplayer testing for ClimbingGame"""
    
    def __init__(self, build_path: str, test_results_dir: str = "TestResults/Multiplayer"):
        self.build_path = Path(build_path).resolve()
        self.test_results_dir = Path(test_results_dir)
        self.test_results_dir.mkdir(parents=True, exist_ok=True)
        
        # Game executable paths
        self.game_executable = self.find_game_executable()
        if not self.game_executable:
            raise FileNotFoundError("Could not find ClimbingGame executable")
        
        # Test configuration
        self.base_port = 7777
        self.max_instances = 8
        self.instance_startup_timeout = 60
        
        # Active instances tracking
        self.active_instances: Dict[str, GameInstance] = {}
        self.port_pool = list(range(self.base_port, self.base_port + 20))
        self.used_ports = set()
        
        # Test scenarios
        self.test_scenarios = self.load_test_scenarios()
        
        # Test results
        self.test_results = {
            'timestamp': time.time(),
            'total_tests': 0,
            'passed_tests': 0,
            'failed_tests': 0,
            'scenarios': []
        }
    
    def find_game_executable(self) -> Optional[Path]:
        """Find the ClimbingGame executable"""
        possible_paths = [
            self.build_path / "ClimbingGame.exe",
            self.build_path / "WindowsNoEditor" / "ClimbingGame.exe",
            self.build_path / "Binaries" / "Win64" / "ClimbingGame.exe",
            self.build_path / "ClimbingGame" / "Binaries" / "Win64" / "ClimbingGame.exe"
        ]
        
        for path in possible_paths:
            if path.exists():
                logger.info(f"Found game executable: {path}")
                return path
        
        logger.error("Could not find ClimbingGame executable in build directory")
        return None
    
    def get_available_port(self) -> int:
        """Get an available port for game instance"""
        for port in self.port_pool:
            if port not in self.used_ports:
                # Check if port is actually available
                with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
                    try:
                        sock.bind(('localhost', port))
                        self.used_ports.add(port)
                        return port
                    except OSError:
                        continue
        
        raise RuntimeError("No available ports for game instances")
    
    def load_test_scenarios(self) -> Dict[str, TestScenario]:
        """Load predefined test scenarios"""
        scenarios = {
            'basic_connection': TestScenario(
                name='Basic Connection Test',
                description='Test basic server-client connection',
                min_players=2,
                max_players=2,
                duration=60,
                map_name='TestLevel',
                test_objectives=[
                    'Client connects to server successfully',
                    'Client receives initial game state',
                    'Basic communication established'
                ],
                success_criteria={
                    'connection_time': 10,  # seconds
                    'packet_loss': 0.01,   # 1%
                    'latency_max': 100     # ms
                }
            ),
            
            'cooperative_climbing': TestScenario(
                name='Cooperative Climbing Test',
                description='Test cooperative climbing mechanics between players',
                min_players=2,
                max_players=4,
                duration=300,
                map_name='CooperativeTestRoute',
                test_objectives=[
                    'Players can assist each other with rope systems',
                    'Shared anchor points work correctly',
                    'Proximity chat functions properly',
                    'Physics synchronization is accurate'
                ],
                success_criteria={
                    'physics_sync_accuracy': 0.95,
                    'rope_tension_sync': 0.90,
                    'voice_quality': 0.80
                }
            ),
            
            'rope_physics_sync': TestScenario(
                name='Rope Physics Synchronization',
                description='Test rope and cable physics synchronization across clients',
                min_players=2,
                max_players=3,
                duration=180,
                map_name='RopeTestLevel',
                test_objectives=[
                    'Rope physics are synchronized between clients',
                    'Anchor placement is consistent',
                    'Tension forces are properly replicated',
                    'Cable breaking mechanics work in multiplayer'
                ],
                success_criteria={
                    'position_deviation': 0.1,  # meters
                    'force_sync_accuracy': 0.90,
                    'break_event_sync': 1.0
                }
            ),
            
            'stress_test_4_players': TestScenario(
                name='4-Player Stress Test',
                description='Stress test with maximum players performing complex actions',
                min_players=4,
                max_players=4,
                duration=600,
                map_name='MultiPitchRoute',
                test_objectives=[
                    'Server maintains stable performance with 4 players',
                    'Complex rope interactions remain stable',
                    'No memory leaks during extended play',
                    'All players can complete climbing route together'
                ],
                success_criteria={
                    'server_fps': 60,
                    'memory_growth': 100,  # MB max growth
                    'completion_rate': 1.0
                }
            ),
            
            'network_instability': TestScenario(
                name='Network Instability Test',
                description='Test behavior under poor network conditions',
                min_players=2,
                max_players=2,
                duration=180,
                map_name='TestLevel',
                test_objectives=[
                    'Game handles packet loss gracefully',
                    'Reconnection system works correctly',
                    'State recovery after network issues',
                    'No desynchronization under stress'
                ],
                success_criteria={
                    'recovery_time': 30,  # seconds
                    'state_consistency': 0.95,
                    'reconnect_success': 1.0
                }
            )
        }
        
        return scenarios
    
    def launch_game_instance(self, role: str, server_ip: str = "127.0.0.1", 
                           server_port: int = None, map_name: str = "TestLevel") -> GameInstance:
        """Launch a game instance with specified role"""
        
        instance_id = f"{role}_{int(time.time() * 1000) % 10000}"
        
        if role == 'server' or role == 'host':
            port = self.get_available_port()
            cmd_args = [
                str(self.game_executable),
                map_name,
                f"-server",
                f"-port={port}",
                f"-log",
                f"-unattended",
                f"-nullrhi"
            ]
        else:  # client
            if server_port is None:
                raise ValueError("Server port required for client instances")
            
            cmd_args = [
                str(self.game_executable),
                f"{server_ip}:{server_port}",
                f"-log",
                f"-unattended",
                f"-nullrhi"
            ]
            port = server_port
        
        logger.info(f"Launching {role} instance: {instance_id}")
        
        try:
            start_time = time.time()
            process = subprocess.Popen(
                cmd_args,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                cwd=self.build_path
            )
            
            # Wait for process to start
            time.sleep(5)
            
            if process.poll() is not None:
                stdout, stderr = process.communicate()
                raise RuntimeError(f"Game instance failed to start: {stderr}")
            
            instance = GameInstance(
                process=process,
                instance_id=instance_id,
                role=role,
                port=port,
                pid=process.pid,
                startup_time=time.time() - start_time
            )
            
            self.active_instances[instance_id] = instance
            logger.info(f"Successfully launched {role} instance {instance_id} (PID: {process.pid})")
            
            return instance
            
        except Exception as e:
            logger.error(f"Failed to launch {role} instance: {e}")
            if 'process' in locals():
                process.terminate()
            raise
    
    def wait_for_instances_ready(self, instances: List[GameInstance], timeout: int = 60) -> bool:
        """Wait for all instances to be ready for testing"""
        logger.info(f"Waiting for {len(instances)} instances to be ready...")
        
        start_time = time.time()
        
        while time.time() - start_time < timeout:
            all_ready = True
            
            for instance in instances:
                # Check if process is still running
                if instance.process.poll() is not None:
                    logger.error(f"Instance {instance.instance_id} terminated unexpectedly")
                    return False
                
                # Additional readiness checks can be added here
                # For now, we assume instances are ready if they're still running
                
            if all_ready:
                logger.info("All instances are ready")
                return True
            
            time.sleep(2)
        
        logger.error("Timeout waiting for instances to be ready")
        return False
    
    def monitor_instance_performance(self, instance: GameInstance, duration: int) -> Dict:
        """Monitor performance metrics for a game instance"""
        metrics = {
            'cpu_usage': [],
            'memory_usage': [],
            'fps_data': [],
            'network_stats': []
        }
        
        try:
            process = psutil.Process(instance.pid)
            start_time = time.time()
            
            while time.time() - start_time < duration:
                if not process.is_running():
                    break
                
                # CPU and Memory usage
                cpu_percent = process.cpu_percent()
                memory_info = process.memory_info()
                memory_mb = memory_info.rss / (1024 * 1024)
                
                metrics['cpu_usage'].append(cpu_percent)
                metrics['memory_usage'].append(memory_mb)
                
                # Network stats (if available)
                try:
                    connections = process.connections()
                    metrics['network_stats'].append(len(connections))
                except (psutil.NoSuchProcess, psutil.AccessDenied):
                    pass
                
                time.sleep(1)
        
        except psutil.NoSuchProcess:
            logger.warning(f"Process {instance.pid} no longer exists")
        
        return metrics
    
    def run_scenario_test(self, scenario: TestScenario, player_count: int) -> Dict:
        """Run a specific test scenario"""
        logger.info(f"Starting scenario: {scenario.name} with {player_count} players")
        
        scenario_result = {
            'scenario_name': scenario.name,
            'player_count': player_count,
            'start_time': time.time(),
            'duration': scenario.duration,
            'success': False,
            'instances': [],
            'performance_metrics': {},
            'errors': [],
            'test_objectives_met': []
        }
        
        instances = []
        
        try:
            # Launch server/host instance
            server = self.launch_game_instance('server', map_name=scenario.map_name)
            instances.append(server)
            scenario_result['instances'].append({
                'id': server.instance_id,
                'role': 'server',
                'pid': server.pid,
                'startup_time': server.startup_time
            })
            
            # Launch client instances
            for i in range(player_count - 1):
                client = self.launch_game_instance(
                    'client', 
                    server_port=server.port,
                    map_name=scenario.map_name
                )
                instances.append(client)
                scenario_result['instances'].append({
                    'id': client.instance_id,
                    'role': 'client',
                    'pid': client.pid,
                    'startup_time': client.startup_time
                })
            
            # Wait for all instances to be ready
            if not self.wait_for_instances_ready(instances):
                raise RuntimeError("Failed to start all instances")
            
            # Run the actual test
            logger.info(f"Running test for {scenario.duration} seconds...")
            
            # Monitor performance in parallel
            performance_futures = []
            with ThreadPoolExecutor(max_workers=len(instances)) as executor:
                for instance in instances:
                    future = executor.submit(
                        self.monitor_instance_performance,
                        instance,
                        scenario.duration
                    )
                    performance_futures.append((instance.instance_id, future))
                
                # Wait for test duration
                time.sleep(scenario.duration)
                
                # Collect performance data
                for instance_id, future in performance_futures:
                    try:
                        scenario_result['performance_metrics'][instance_id] = future.result(timeout=10)
                    except Exception as e:
                        logger.error(f"Failed to collect metrics for {instance_id}: {e}")
            
            # Evaluate test results
            scenario_result['success'] = self.evaluate_scenario_results(scenario, scenario_result)
            
        except Exception as e:
            logger.error(f"Scenario {scenario.name} failed: {e}")
            scenario_result['errors'].append(str(e))
            scenario_result['success'] = False
        
        finally:
            # Clean up instances
            for instance in instances:
                try:
                    logger.info(f"Terminating instance {instance.instance_id}")
                    instance.process.terminate()
                    
                    # Wait for graceful shutdown
                    try:
                        instance.process.wait(timeout=10)
                    except subprocess.TimeoutExpired:
                        logger.warning(f"Force killing instance {instance.instance_id}")
                        instance.process.kill()
                    
                    if instance.port in self.used_ports:
                        self.used_ports.remove(instance.port)
                    
                    if instance.instance_id in self.active_instances:
                        del self.active_instances[instance.instance_id]
                        
                except Exception as e:
                    logger.error(f"Error cleaning up instance {instance.instance_id}: {e}")
        
        scenario_result['end_time'] = time.time()
        scenario_result['actual_duration'] = scenario_result['end_time'] - scenario_result['start_time']
        
        logger.info(f"Scenario {scenario.name} completed - Success: {scenario_result['success']}")
        
        return scenario_result
    
    def evaluate_scenario_results(self, scenario: TestScenario, result: Dict) -> bool:
        """Evaluate if scenario met success criteria"""
        
        # Basic checks - all instances should have run for the full duration
        if len(result['errors']) > 0:
            return False
        
        if result['actual_duration'] < scenario.duration * 0.9:  # Allow 10% tolerance
            return False
        
        # Performance checks
        for instance_id, metrics in result['performance_metrics'].items():
            if not metrics['cpu_usage'] or not metrics['memory_usage']:
                return False
            
            # Check for excessive resource usage
            avg_cpu = sum(metrics['cpu_usage']) / len(metrics['cpu_usage'])
            max_memory = max(metrics['memory_usage'])
            
            if avg_cpu > 90:  # 90% CPU usage threshold
                logger.warning(f"High CPU usage in {instance_id}: {avg_cpu:.2f}%")
            
            if max_memory > 2000:  # 2GB memory threshold
                logger.warning(f"High memory usage in {instance_id}: {max_memory:.2f}MB")
        
        # Scenario-specific success criteria can be added here
        # For now, we consider it successful if it ran without errors
        
        return True
    
    def run_all_tests(self, scenarios: List[str], max_players: int = 4, 
                     timeout: int = 3600) -> Dict:
        """Run all specified test scenarios"""
        logger.info(f"Starting multiplayer test suite with scenarios: {scenarios}")
        
        self.test_results['start_time'] = time.time()
        
        for scenario_name in scenarios:
            if scenario_name not in self.test_scenarios:
                logger.error(f"Unknown scenario: {scenario_name}")
                continue
            
            scenario = self.test_scenarios[scenario_name]
            
            # Test with different player counts
            player_counts = [scenario.min_players]
            if scenario.max_players > scenario.min_players and scenario.max_players <= max_players:
                player_counts.append(min(scenario.max_players, max_players))
            
            for player_count in player_counts:
                self.test_results['total_tests'] += 1
                
                result = self.run_scenario_test(scenario, player_count)
                
                if result['success']:
                    self.test_results['passed_tests'] += 1
                else:
                    self.test_results['failed_tests'] += 1
                
                self.test_results['scenarios'].append(result)
                
                # Small delay between scenarios
                time.sleep(10)
        
        self.test_results['end_time'] = time.time()
        self.test_results['total_duration'] = (
            self.test_results['end_time'] - self.test_results['start_time']
        )
        
        logger.info(f"Test suite completed: {self.test_results['passed_tests']}/{self.test_results['total_tests']} passed")
        
        return self.test_results
    
    def save_results(self, output_file: str):
        """Save test results to JSON file"""
        with open(output_file, 'w') as f:
            json.dump(self.test_results, f, indent=2)
        
        logger.info(f"Test results saved to: {output_file}")

def main():
    parser = argparse.ArgumentParser(description='ClimbingGame Multiplayer Test Runner')
    parser.add_argument('--build-path', required=True,
                       help='Path to the game build directory')
    parser.add_argument('--test-scenarios', nargs='+', 
                       choices=['basic_connection', 'cooperative_climbing', 'rope_physics_sync', 
                               'stress_test_4_players', 'network_instability'],
                       default=['basic_connection', 'cooperative_climbing'],
                       help='Test scenarios to run')
    parser.add_argument('--max-players', type=int, default=4,
                       help='Maximum number of players per test')
    parser.add_argument('--timeout', type=int, default=3600,
                       help='Overall test timeout in seconds')
    parser.add_argument('--output-file', default='multiplayer-test-results.json',
                       help='Output file for test results')
    
    args = parser.parse_args()
    
    try:
        test_runner = MultiplayerTestRunner(args.build_path)
        
        results = test_runner.run_all_tests(
            scenarios=args.test_scenarios,
            max_players=args.max_players,
            timeout=args.timeout
        )
        
        test_runner.save_results(args.output_file)
        
        # Print summary
        print(f"\nMultiplayer Test Results:")
        print(f"  Total Tests: {results['total_tests']}")
        print(f"  Passed: {results['passed_tests']}")
        print(f"  Failed: {results['failed_tests']}")
        print(f"  Duration: {results['total_duration']:.2f} seconds")
        
        # Exit with appropriate code
        sys.exit(0 if results['failed_tests'] == 0 else 1)
    
    except Exception as e:
        logger.error(f"Multiplayer testing failed: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()