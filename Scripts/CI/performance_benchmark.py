#!/usr/bin/env python3
"""
ClimbingGame Performance Benchmarking System
Automated performance testing and regression detection for physics-heavy gameplay
"""

import os
import sys
import json
import time
import logging
import argparse
import subprocess
import threading
from pathlib import Path
from typing import Dict, List, Optional, Tuple
import psutil
import csv
from dataclasses import dataclass, asdict
from statistics import mean, stdev
import matplotlib.pyplot as plt
import matplotlib
matplotlib.use('Agg')  # Use non-interactive backend

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

@dataclass
class PerformanceMetrics:
    """Container for performance metrics"""
    timestamp: float
    fps: float
    frame_time: float
    cpu_usage: float
    memory_usage: float  # MB
    gpu_usage: float
    vram_usage: float   # MB
    draw_calls: int
    triangles: int
    physics_objects: int
    rope_segments: int
    network_objects: int

@dataclass
class BenchmarkScenario:
    """Defines a performance benchmark scenario"""
    name: str
    description: str
    map_name: str
    duration: int  # seconds
    warmup_time: int  # seconds
    player_count: int
    scenario_commands: List[str]
    expected_fps_min: float
    expected_memory_max: float  # MB

class PerformanceBenchmarkRunner:
    """Handles automated performance benchmarking for ClimbingGame"""
    
    def __init__(self, build_path: str, output_dir: str = "performance-results"):
        self.build_path = Path(build_path).resolve()
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(parents=True, exist_ok=True)
        
        # Game executable
        self.game_executable = self.find_game_executable()
        if not self.game_executable:
            raise FileNotFoundError("Could not find ClimbingGame executable")
        
        # Benchmark configuration
        self.sample_interval = 0.5  # seconds
        self.baseline_data_path = self.output_dir / "baseline_performance.json"
        
        # Performance monitoring
        self.metrics_history: List[PerformanceMetrics] = []
        self.is_monitoring = False
        self.monitor_thread: Optional[threading.Thread] = None
        
        # Benchmark scenarios
        self.scenarios = self.load_benchmark_scenarios()
        
        # Results
        self.benchmark_results = {
            'timestamp': time.time(),
            'build_info': self.get_build_info(),
            'system_info': self.get_system_info(),
            'scenarios': []
        }
    
    def find_game_executable(self) -> Optional[Path]:
        """Find the ClimbingGame executable"""
        possible_paths = [
            self.build_path / "ClimbingGame.exe",
            self.build_path / "WindowsNoEditor" / "ClimbingGame.exe",
            self.build_path / "Binaries" / "Win64" / "ClimbingGame.exe"
        ]
        
        for path in possible_paths:
            if path.exists():
                return path
        
        return None
    
    def get_build_info(self) -> Dict:
        """Get build information"""
        # In a real implementation, this would parse build metadata
        return {
            'build_path': str(self.build_path),
            'executable_size': self.game_executable.stat().st_size if self.game_executable else 0,
            'build_timestamp': time.ctime(self.game_executable.stat().st_mtime) if self.game_executable else None
        }
    
    def get_system_info(self) -> Dict:
        """Get system hardware information"""
        return {
            'cpu_count': psutil.cpu_count(),
            'cpu_freq': psutil.cpu_freq()._asdict() if psutil.cpu_freq() else {},
            'memory_total': psutil.virtual_memory().total / (1024**3),  # GB
            'platform': sys.platform,
            'python_version': sys.version
        }
    
    def load_benchmark_scenarios(self) -> Dict[str, BenchmarkScenario]:
        """Load performance benchmark scenarios"""
        scenarios = {
            'climbing_physics': BenchmarkScenario(
                name='Climbing Physics Stress Test',
                description='Tests physics performance with complex climbing mechanics',
                map_name='PhysicsTestLevel',
                duration=300,  # 5 minutes
                warmup_time=30,
                player_count=1,
                scenario_commands=[
                    'climb.spawn_player 0 0 0',
                    'climb.set_stamina 100',
                    'physics.spawn_ropes 10',
                    'climb.auto_climb_complex_route'
                ],
                expected_fps_min=60.0,
                expected_memory_max=2000.0
            ),
            
            'rope_simulation': BenchmarkScenario(
                name='Rope Physics Simulation',
                description='Stress test for cable and rope physics systems',
                map_name='RopeTestLevel',
                duration=180,
                warmup_time=15,
                player_count=1,
                scenario_commands=[
                    'physics.spawn_ropes 50',
                    'physics.set_rope_segments 20',
                    'physics.simulate_wind_stress',
                    'physics.random_rope_interactions'
                ],
                expected_fps_min=45.0,
                expected_memory_max=1500.0
            ),
            
            'multiplayer_stress': BenchmarkScenario(
                name='Multiplayer Networking Stress',
                description='Tests network performance with multiple players',
                map_name='MultiplayerTestLevel',
                duration=240,
                warmup_time=30,
                player_count=4,
                scenario_commands=[
                    'mp.spawn_players 4',
                    'mp.simulate_cooperative_climbing',
                    'physics.spawn_shared_ropes 20',
                    'mp.stress_test_sync'
                ],
                expected_fps_min=30.0,
                expected_memory_max=3000.0
            ),
            
            'large_environment': BenchmarkScenario(
                name='Large Environment Stress Test',
                description='Tests rendering performance with large detailed environments',
                map_name='LargeEnvironmentLevel',
                duration=300,
                warmup_time=45,
                player_count=1,
                scenario_commands=[
                    'render.set_view_distance_max',
                    'render.enable_all_effects',
                    'environment.load_full_detail',
                    'climb.traverse_large_route'
                ],
                expected_fps_min=60.0,
                expected_memory_max=4000.0
            ),
            
            'memory_stress': BenchmarkScenario(
                name='Memory Usage Stress Test',
                description='Tests for memory leaks and excessive usage',
                map_name='MemoryTestLevel',
                duration=600,  # 10 minutes
                warmup_time=60,
                player_count=1,
                scenario_commands=[
                    'memory.stress_test_allocations',
                    'physics.spawn_many_objects 1000',
                    'audio.play_multiple_sounds',
                    'render.cycle_quality_settings'
                ],
                expected_fps_min=30.0,
                expected_memory_max=2500.0
            )
        }
        
        return scenarios
    
    def launch_game_for_benchmark(self, scenario: BenchmarkScenario) -> subprocess.Popen:
        """Launch game instance configured for benchmarking"""
        
        cmd_args = [
            str(self.game_executable),
            scenario.map_name,
            '-benchmark',
            '-unattended',
            '-log',
            '-windowed',
            '-ResX=1920',
            '-ResY=1080',
            f'-ExecCmds=stat fps; stat unit'  # Enable FPS and timing stats
        ]
        
        # Add scenario-specific commands
        if scenario.scenario_commands:
            cmd_str = '; '.join(scenario.scenario_commands)
            cmd_args.append(f'-ExecCmds={cmd_str}')
        
        logger.info(f"Launching benchmark for scenario: {scenario.name}")
        
        process = subprocess.Popen(
            cmd_args,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            cwd=self.build_path
        )
        
        return process
    
    def start_performance_monitoring(self, process: subprocess.Popen):
        """Start monitoring performance metrics"""
        self.metrics_history.clear()
        self.is_monitoring = True
        
        def monitor_loop():
            try:
                game_process = psutil.Process(process.pid)
                
                while self.is_monitoring and process.poll() is None:
                    try:
                        # Basic system metrics
                        cpu_percent = game_process.cpu_percent()
                        memory_info = game_process.memory_info()
                        memory_mb = memory_info.rss / (1024 * 1024)
                        
                        # GPU metrics (requires additional libraries like nvidia-ml-py)
                        # For now, we'll use placeholder values
                        gpu_usage = 0.0
                        vram_usage = 0.0
                        
                        # Game-specific metrics would need to be extracted from log files
                        # or through engine APIs. For now, we'll use estimated values
                        fps = 60.0  # Would be parsed from game logs
                        frame_time = 1000.0 / fps  # ms
                        draw_calls = 1000  # Estimated
                        triangles = 100000  # Estimated
                        physics_objects = 50  # Estimated
                        rope_segments = 200  # Estimated
                        network_objects = 0  # Would vary based on multiplayer
                        
                        metrics = PerformanceMetrics(
                            timestamp=time.time(),
                            fps=fps,
                            frame_time=frame_time,
                            cpu_usage=cpu_percent,
                            memory_usage=memory_mb,
                            gpu_usage=gpu_usage,
                            vram_usage=vram_usage,
                            draw_calls=draw_calls,
                            triangles=triangles,
                            physics_objects=physics_objects,
                            rope_segments=rope_segments,
                            network_objects=network_objects
                        )
                        
                        self.metrics_history.append(metrics)
                        
                    except psutil.NoSuchProcess:
                        logger.warning("Game process terminated")
                        break
                    except Exception as e:
                        logger.error(f"Error monitoring performance: {e}")
                    
                    time.sleep(self.sample_interval)
                    
            except Exception as e:
                logger.error(f"Performance monitoring error: {e}")
        
        self.monitor_thread = threading.Thread(target=monitor_loop)
        self.monitor_thread.start()
    
    def stop_performance_monitoring(self):
        """Stop performance monitoring"""
        self.is_monitoring = False
        if self.monitor_thread:
            self.monitor_thread.join(timeout=5)
    
    def analyze_performance_data(self, scenario: BenchmarkScenario) -> Dict:
        """Analyze collected performance metrics"""
        if not self.metrics_history:
            return {'error': 'No performance data collected'}
        
        # Skip warmup period
        warmup_samples = int(scenario.warmup_time / self.sample_interval)
        relevant_metrics = self.metrics_history[warmup_samples:]
        
        if not relevant_metrics:
            return {'error': 'No metrics after warmup period'}
        
        # Calculate statistics
        fps_values = [m.fps for m in relevant_metrics]
        frame_time_values = [m.frame_time for m in relevant_metrics]
        cpu_values = [m.cpu_usage for m in relevant_metrics]
        memory_values = [m.memory_usage for m in relevant_metrics]
        
        analysis = {
            'scenario_name': scenario.name,
            'sample_count': len(relevant_metrics),
            'duration': len(relevant_metrics) * self.sample_interval,
            
            'fps': {
                'min': min(fps_values),
                'max': max(fps_values),
                'mean': mean(fps_values),
                'stdev': stdev(fps_values) if len(fps_values) > 1 else 0,
                'percentile_1': sorted(fps_values)[int(len(fps_values) * 0.01)],
                'percentile_5': sorted(fps_values)[int(len(fps_values) * 0.05)],
                'percentile_95': sorted(fps_values)[int(len(fps_values) * 0.95)]
            },
            
            'frame_time': {
                'min': min(frame_time_values),
                'max': max(frame_time_values),
                'mean': mean(frame_time_values),
                'stdev': stdev(frame_time_values) if len(frame_time_values) > 1 else 0
            },
            
            'cpu_usage': {
                'min': min(cpu_values),
                'max': max(cpu_values),
                'mean': mean(cpu_values),
                'stdev': stdev(cpu_values) if len(cpu_values) > 1 else 0
            },
            
            'memory_usage': {
                'min': min(memory_values),
                'max': max(memory_values),
                'mean': mean(memory_values),
                'stdev': stdev(memory_values) if len(memory_values) > 1 else 0,
                'growth': max(memory_values) - min(memory_values)  # Memory growth during test
            },
            
            'performance_score': self.calculate_performance_score(scenario, relevant_metrics),
            'meets_expectations': self.check_performance_expectations(scenario, relevant_metrics),
            'issues_detected': self.detect_performance_issues(scenario, relevant_metrics)
        }
        
        return analysis
    
    def calculate_performance_score(self, scenario: BenchmarkScenario, 
                                   metrics: List[PerformanceMetrics]) -> float:
        """Calculate overall performance score (0-100)"""
        if not metrics:
            return 0.0
        
        fps_values = [m.fps for m in metrics]
        memory_values = [m.memory_usage for m in metrics]
        
        # FPS score (0-50 points)
        avg_fps = mean(fps_values)
        fps_score = min(50, (avg_fps / scenario.expected_fps_min) * 50)
        
        # Memory score (0-25 points)
        max_memory = max(memory_values)
        memory_score = max(0, 25 - ((max_memory - scenario.expected_memory_max) / 100))
        
        # Stability score (0-25 points)
        fps_stdev = stdev(fps_values) if len(fps_values) > 1 else 0
        stability_score = max(0, 25 - fps_stdev)
        
        total_score = fps_score + memory_score + stability_score
        return min(100.0, max(0.0, total_score))
    
    def check_performance_expectations(self, scenario: BenchmarkScenario, 
                                     metrics: List[PerformanceMetrics]) -> Dict:
        """Check if performance meets expectations"""
        if not metrics:
            return {'fps': False, 'memory': False}
        
        fps_values = [m.fps for m in metrics]
        memory_values = [m.memory_usage for m in metrics]
        
        min_fps = min(fps_values)
        max_memory = max(memory_values)
        
        return {
            'fps': min_fps >= scenario.expected_fps_min,
            'memory': max_memory <= scenario.expected_memory_max,
            'fps_actual': min_fps,
            'memory_actual': max_memory
        }
    
    def detect_performance_issues(self, scenario: BenchmarkScenario, 
                                 metrics: List[PerformanceMetrics]) -> List[str]:
        """Detect potential performance issues"""
        issues = []
        
        if not metrics:
            return ['No performance data collected']
        
        fps_values = [m.fps for m in metrics]
        memory_values = [m.memory_usage for m in metrics]
        cpu_values = [m.cpu_usage for m in metrics]
        
        # FPS issues
        if min(fps_values) < 15:
            issues.append("Severe FPS drops detected (below 15 FPS)")
        elif min(fps_values) < 30:
            issues.append("Significant FPS drops detected (below 30 FPS)")
        
        if stdev(fps_values) > 10 if len(fps_values) > 1 else False:
            issues.append("High FPS variance detected (unstable framerate)")
        
        # Memory issues
        memory_growth = max(memory_values) - min(memory_values)
        if memory_growth > 500:  # MB
            issues.append(f"Significant memory growth detected ({memory_growth:.1f}MB)")
        
        if max(memory_values) > 4000:  # MB
            issues.append("High memory usage detected (above 4GB)")
        
        # CPU issues
        if mean(cpu_values) > 90:
            issues.append("High average CPU usage (above 90%)")
        
        return issues
    
    def generate_performance_graphs(self, scenario_name: str, analysis: Dict):
        """Generate performance visualization graphs"""
        if not self.metrics_history:
            return
        
        # Create figure with subplots
        fig, ((ax1, ax2), (ax3, ax4)) = plt.subplots(2, 2, figsize=(15, 10))
        fig.suptitle(f'Performance Analysis: {scenario_name}')
        
        timestamps = [(m.timestamp - self.metrics_history[0].timestamp) for m in self.metrics_history]
        
        # FPS graph
        fps_values = [m.fps for m in self.metrics_history]
        ax1.plot(timestamps, fps_values, 'b-', linewidth=1)
        ax1.set_title('FPS over Time')
        ax1.set_xlabel('Time (seconds)')
        ax1.set_ylabel('FPS')
        ax1.grid(True, alpha=0.3)
        
        # Memory usage graph
        memory_values = [m.memory_usage for m in self.metrics_history]
        ax2.plot(timestamps, memory_values, 'r-', linewidth=1)
        ax2.set_title('Memory Usage over Time')
        ax2.set_xlabel('Time (seconds)')
        ax2.set_ylabel('Memory (MB)')
        ax2.grid(True, alpha=0.3)
        
        # CPU usage graph
        cpu_values = [m.cpu_usage for m in self.metrics_history]
        ax3.plot(timestamps, cpu_values, 'g-', linewidth=1)
        ax3.set_title('CPU Usage over Time')
        ax3.set_xlabel('Time (seconds)')
        ax3.set_ylabel('CPU %')
        ax3.grid(True, alpha=0.3)
        
        # Frame time histogram
        frame_time_values = [m.frame_time for m in self.metrics_history]
        ax4.hist(frame_time_values, bins=50, alpha=0.7, color='purple')
        ax4.set_title('Frame Time Distribution')
        ax4.set_xlabel('Frame Time (ms)')
        ax4.set_ylabel('Frequency')
        ax4.grid(True, alpha=0.3)
        
        plt.tight_layout()
        
        graph_path = self.output_dir / f"{scenario_name.replace(' ', '_').lower()}_performance.png"
        plt.savefig(graph_path, dpi=150, bbox_inches='tight')
        plt.close()
        
        logger.info(f"Performance graph saved: {graph_path}")
    
    def run_benchmark_scenario(self, scenario_name: str) -> Dict:
        """Run a single benchmark scenario"""
        if scenario_name not in self.scenarios:
            raise ValueError(f"Unknown scenario: {scenario_name}")
        
        scenario = self.scenarios[scenario_name]
        logger.info(f"Starting benchmark scenario: {scenario.name}")
        
        result = {
            'scenario_name': scenario.name,
            'start_time': time.time(),
            'success': False,
            'error': None
        }
        
        game_process = None
        
        try:
            # Launch game
            game_process = self.launch_game_for_benchmark(scenario)
            
            # Wait for game to start
            time.sleep(10)
            
            if game_process.poll() is not None:
                stdout, stderr = game_process.communicate()
                raise RuntimeError(f"Game failed to start: {stderr}")
            
            # Start performance monitoring
            self.start_performance_monitoring(game_process)
            
            # Wait for benchmark to complete
            total_time = scenario.warmup_time + scenario.duration
            logger.info(f"Running benchmark for {total_time} seconds...")
            
            start_time = time.time()
            while time.time() - start_time < total_time:
                if game_process.poll() is not None:
                    logger.warning("Game process terminated early")
                    break
                time.sleep(1)
            
            # Stop monitoring
            self.stop_performance_monitoring()
            
            # Analyze results
            analysis = self.analyze_performance_data(scenario)
            result.update(analysis)
            
            # Generate graphs
            self.generate_performance_graphs(scenario.name, analysis)
            
            result['success'] = True
            
        except Exception as e:
            logger.error(f"Benchmark scenario failed: {e}")
            result['error'] = str(e)
            
        finally:
            # Clean up
            self.stop_performance_monitoring()
            
            if game_process:
                try:
                    game_process.terminate()
                    game_process.wait(timeout=10)
                except subprocess.TimeoutExpired:
                    game_process.kill()
                except Exception as e:
                    logger.error(f"Error terminating game process: {e}")
        
        result['end_time'] = time.time()
        result['total_duration'] = result['end_time'] - result['start_time']
        
        return result
    
    def run_all_benchmarks(self, scenarios: List[str]) -> Dict:
        """Run all specified benchmark scenarios"""
        logger.info(f"Starting performance benchmarks for scenarios: {scenarios}")
        
        self.benchmark_results['scenarios'] = []
        
        for scenario_name in scenarios:
            try:
                result = self.run_benchmark_scenario(scenario_name)
                self.benchmark_results['scenarios'].append(result)
                
                # Small delay between scenarios
                time.sleep(30)
                
            except Exception as e:
                logger.error(f"Failed to run scenario {scenario_name}: {e}")
                self.benchmark_results['scenarios'].append({
                    'scenario_name': scenario_name,
                    'success': False,
                    'error': str(e)
                })
        
        return self.benchmark_results
    
    def save_results(self, output_file: str):
        """Save benchmark results to JSON file"""
        with open(output_file, 'w') as f:
            json.dump(self.benchmark_results, f, indent=2)
        
        logger.info(f"Benchmark results saved to: {output_file}")
    
    def export_csv_report(self, output_file: str):
        """Export detailed metrics to CSV"""
        if not self.metrics_history:
            return
        
        with open(output_file, 'w', newline='') as csvfile:
            fieldnames = [
                'timestamp', 'fps', 'frame_time', 'cpu_usage', 'memory_usage',
                'gpu_usage', 'vram_usage', 'draw_calls', 'triangles',
                'physics_objects', 'rope_segments', 'network_objects'
            ]
            
            writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
            writer.writeheader()
            
            for metrics in self.metrics_history:
                writer.writerow(asdict(metrics))
        
        logger.info(f"CSV report exported to: {output_file}")

def main():
    parser = argparse.ArgumentParser(description='ClimbingGame Performance Benchmark Runner')
    parser.add_argument('--build-path', required=True,
                       help='Path to the game build directory')
    parser.add_argument('--test-duration', type=int, default=300,
                       help='Duration for each test scenario (seconds)')
    parser.add_argument('--scenarios', nargs='+',
                       choices=['climbing_physics', 'rope_simulation', 'multiplayer_stress', 
                               'large_environment', 'memory_stress'],
                       default=['climbing_physics', 'rope_simulation'],
                       help='Benchmark scenarios to run')
    parser.add_argument('--output-dir', default='performance-results',
                       help='Directory for output files')
    
    args = parser.parse_args()
    
    try:
        benchmark_runner = PerformanceBenchmarkRunner(args.build_path, args.output_dir)
        
        results = benchmark_runner.run_all_benchmarks(args.scenarios)
        
        # Save results
        output_file = Path(args.output_dir) / 'benchmark_results.json'
        benchmark_runner.save_results(str(output_file))
        
        # Export CSV
        csv_file = Path(args.output_dir) / 'benchmark_metrics.csv'
        benchmark_runner.export_csv_report(str(csv_file))
        
        # Print summary
        print(f"\nPerformance Benchmark Results:")
        successful_scenarios = [s for s in results['scenarios'] if s.get('success', False)]
        print(f"  Completed: {len(successful_scenarios)}/{len(results['scenarios'])} scenarios")
        
        for scenario_result in successful_scenarios:
            score = scenario_result.get('performance_score', 0)
            print(f"  {scenario_result['scenario_name']}: Score {score:.1f}/100")
        
        # Exit with appropriate code
        all_successful = len(successful_scenarios) == len(results['scenarios'])
        sys.exit(0 if all_successful else 1)
    
    except Exception as e:
        logger.error(f"Performance benchmarking failed: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()