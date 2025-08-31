#!/usr/bin/env python3
"""
ClimbingGame Performance Regression Detection System
Automated regression testing against established performance baselines
"""

import os
import sys
import json
import logging
import argparse
import subprocess
from pathlib import Path
from typing import Dict, List, Optional, Tuple, Any
from dataclasses import dataclass, asdict
import numpy as np
from statistics import mean, stdev
import requests
import tempfile
import shutil

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

@dataclass
class RegressionTest:
    """Defines a regression test configuration"""
    name: str
    baseline_name: str
    test_scenario: str
    regression_threshold: float  # percentage
    critical_threshold: float   # percentage
    required_metrics: List[str]
    weight: float = 1.0  # Test importance weight

@dataclass
class MetricComparison:
    """Results of comparing a metric against baseline"""
    metric_name: str
    baseline_value: float
    current_value: float
    change_percent: float
    is_regression: bool
    is_critical: bool
    threshold_used: float

@dataclass
class RegressionResult:
    """Results of a complete regression analysis"""
    test_name: str
    baseline_name: str
    overall_passed: bool
    performance_score: float
    metric_comparisons: List[MetricComparison]
    issues_found: List[str]
    recommendations: List[str]
    timestamp: float

class PerformanceRegressionAnalyzer:
    """Analyzes performance results against established baselines"""
    
    def __init__(self, baseline_dir: str = "Saved/PerformanceBaselines", 
                 current_results_dir: str = "performance-results"):
        self.baseline_dir = Path(baseline_dir)
        self.current_results_dir = Path(current_results_dir)
        
        # Load baseline data
        self.baselines = self.load_all_baselines()
        
        # Define regression tests
        self.regression_tests = self.define_regression_tests()
        
        # Analysis configuration
        self.default_regression_threshold = 5.0  # 5% regression threshold
        self.default_critical_threshold = 15.0   # 15% critical threshold
        
        # Results
        self.regression_results: List[RegressionResult] = []
        
    def load_all_baselines(self) -> Dict[str, Dict]:
        """Load all available performance baselines"""
        baselines = {}
        
        if not self.baseline_dir.exists():
            logger.warning(f"Baseline directory not found: {self.baseline_dir}")
            return baselines
        
        for baseline_file in self.baseline_dir.glob("*.json"):
            try:
                with open(baseline_file, 'r') as f:
                    baseline_data = json.load(f)
                    baseline_name = baseline_file.stem
                    baselines[baseline_name] = baseline_data
                    logger.debug(f"Loaded baseline: {baseline_name}")
            except Exception as e:
                logger.error(f"Failed to load baseline {baseline_file}: {e}")
        
        logger.info(f"Loaded {len(baselines)} performance baselines")
        return baselines
    
    def define_regression_tests(self) -> List[RegressionTest]:
        """Define the regression tests to be performed"""
        return [
            RegressionTest(
                name="FPS Performance",
                baseline_name="Windows_RopePhysics_Recommended",
                test_scenario="rope_physics",
                regression_threshold=5.0,
                critical_threshold=15.0,
                required_metrics=["fps_mean", "fps_min", "fps_percentile_1"],
                weight=2.0  # High importance
            ),
            RegressionTest(
                name="Memory Usage",
                baseline_name="Windows_Memory_Recommended",
                test_scenario="memory_stress",
                regression_threshold=10.0,
                critical_threshold=25.0,
                required_metrics=["memory_max", "memory_growth"],
                weight=1.5
            ),
            RegressionTest(
                name="Physics Performance", 
                baseline_name="Windows_RopePhysics_Recommended",
                test_scenario="climbing_physics",
                regression_threshold=8.0,
                critical_threshold=20.0,
                required_metrics=["fps_mean", "frame_time_max"],
                weight=2.0
            ),
            RegressionTest(
                name="Multiplayer Sync",
                baseline_name="Windows_MultiplayerSync_Recommended",
                test_scenario="multiplayer_stress",
                regression_threshold=10.0,
                critical_threshold=30.0,
                required_metrics=["fps_mean", "memory_max"],
                weight=1.0
            ),
            RegressionTest(
                name="Large Environment",
                baseline_name="Windows_Rendering_Recommended", 
                test_scenario="large_environment",
                regression_threshold=12.0,
                critical_threshold=25.0,
                required_metrics=["fps_mean", "fps_percentile_5"],
                weight=1.0
            )
        ]
    
    def load_current_results(self, results_file: str) -> Dict:
        """Load current benchmark results"""
        try:
            with open(results_file, 'r') as f:
                return json.load(f)
        except Exception as e:
            logger.error(f"Failed to load current results from {results_file}: {e}")
            return {}
    
    def extract_metric_value(self, data: Dict, metric_path: str) -> Optional[float]:
        """Extract a metric value from nested data structure"""
        try:
            keys = metric_path.split('.')
            current = data
            for key in keys:
                if isinstance(current, dict) and key in current:
                    current = current[key]
                else:
                    return None
            return float(current) if current is not None else None
        except (ValueError, TypeError):
            return None
    
    def compare_metric(self, baseline_value: float, current_value: float, 
                      regression_threshold: float, critical_threshold: float,
                      metric_name: str, higher_is_better: bool = True) -> MetricComparison:
        """Compare a metric against its baseline"""
        
        if baseline_value == 0:
            change_percent = 0.0
        else:
            change_percent = ((current_value - baseline_value) / baseline_value) * 100.0
        
        # Determine if this is a regression based on whether higher is better
        if higher_is_better:
            # For metrics like FPS, negative change is bad
            is_regression = change_percent < -regression_threshold
            is_critical = change_percent < -critical_threshold
        else:
            # For metrics like memory usage, positive change is bad
            is_regression = change_percent > regression_threshold
            is_critical = change_percent > critical_threshold
        
        return MetricComparison(
            metric_name=metric_name,
            baseline_value=baseline_value,
            current_value=current_value,
            change_percent=change_percent,
            is_regression=is_regression,
            is_critical=is_critical,
            threshold_used=critical_threshold if is_critical else regression_threshold
        )
    
    def analyze_regression_test(self, test: RegressionTest, current_results: Dict) -> RegressionResult:
        """Analyze a single regression test"""
        logger.info(f"Analyzing regression test: {test.name}")
        
        # Get baseline data
        baseline_data = self.baselines.get(test.baseline_name)
        if not baseline_data:
            logger.error(f"Baseline not found: {test.baseline_name}")
            return RegressionResult(
                test_name=test.name,
                baseline_name=test.baseline_name,
                overall_passed=False,
                performance_score=0.0,
                metric_comparisons=[],
                issues_found=[f"Baseline '{test.baseline_name}' not found"],
                recommendations=["Establish baseline before running regression tests"],
                timestamp=0.0
            )
        
        # Find current test results for this scenario
        current_scenario_results = None
        for scenario in current_results.get('scenarios', []):
            if test.test_scenario in scenario.get('scenario_name', '').lower().replace(' ', '_'):
                current_scenario_results = scenario
                break
        
        if not current_scenario_results:
            logger.error(f"Current results not found for scenario: {test.test_scenario}")
            return RegressionResult(
                test_name=test.name,
                baseline_name=test.baseline_name,
                overall_passed=False,
                performance_score=0.0,
                metric_comparisons=[],
                issues_found=[f"No current results found for scenario '{test.test_scenario}'"],
                recommendations=["Run the required benchmark scenario"],
                timestamp=0.0
            )
        
        # Compare metrics
        metric_comparisons = []
        issues_found = []
        recommendations = []
        
        # Define metric mappings and whether higher is better
        metric_mappings = {
            'fps_mean': ('fps.mean', True),
            'fps_min': ('fps.min', True),
            'fps_max': ('fps.max', True),
            'fps_percentile_1': ('fps.percentile_1', True),
            'fps_percentile_5': ('fps.percentile_5', True),
            'fps_percentile_95': ('fps.percentile_95', True),
            'frame_time_mean': ('frame_time.mean', False),
            'frame_time_max': ('frame_time.max', False),
            'memory_max': ('memory_usage.max', False),
            'memory_mean': ('memory_usage.mean', False),
            'memory_growth': ('memory_usage.growth', False),
            'cpu_max': ('cpu_usage.max', False),
            'cpu_mean': ('cpu_usage.mean', False)
        }
        
        for metric_name in test.required_metrics:
            if metric_name not in metric_mappings:
                logger.warning(f"Unknown metric: {metric_name}")
                continue
            
            data_path, higher_is_better = metric_mappings[metric_name]
            
            # Get baseline value
            baseline_value = self.extract_metric_value(baseline_data, metric_name.replace('_', ''))
            if baseline_value is None:
                # Try alternative mappings
                baseline_value = self.extract_metric_value(baseline_data, f"Target{metric_name.split('_')[0].upper()}")
                if baseline_value is None:
                    logger.warning(f"Baseline value not found for {metric_name}")
                    continue
            
            # Get current value
            current_value = self.extract_metric_value(current_scenario_results, data_path)
            if current_value is None:
                logger.warning(f"Current value not found for {metric_name}")
                continue
            
            # Compare
            comparison = self.compare_metric(
                baseline_value, current_value,
                test.regression_threshold, test.critical_threshold,
                metric_name, higher_is_better
            )
            metric_comparisons.append(comparison)
            
            # Add issues and recommendations
            if comparison.is_critical:
                issues_found.append(f"CRITICAL: {metric_name} changed by {comparison.change_percent:.1f}% "
                                  f"(threshold: {test.critical_threshold}%)")
                recommendations.append(f"Investigate {metric_name} performance degradation immediately")
            elif comparison.is_regression:
                issues_found.append(f"REGRESSION: {metric_name} changed by {comparison.change_percent:.1f}% "
                                  f"(threshold: {test.regression_threshold}%)")
                recommendations.append(f"Review changes affecting {metric_name} performance")
        
        # Calculate overall performance score
        performance_score = self.calculate_performance_score(metric_comparisons, test.weight)
        
        # Determine overall pass/fail
        has_critical_issues = any(comp.is_critical for comp in metric_comparisons)
        overall_passed = not has_critical_issues and performance_score >= 70.0
        
        result = RegressionResult(
            test_name=test.name,
            baseline_name=test.baseline_name,
            overall_passed=overall_passed,
            performance_score=performance_score,
            metric_comparisons=metric_comparisons,
            issues_found=issues_found,
            recommendations=recommendations,
            timestamp=current_results.get('timestamp', 0.0)
        )
        
        return result
    
    def calculate_performance_score(self, comparisons: List[MetricComparison], weight: float) -> float:
        """Calculate weighted performance score (0-100)"""
        if not comparisons:
            return 0.0
        
        total_score = 0.0
        
        for comparison in comparisons:
            # Base score starts at 100
            metric_score = 100.0
            
            if comparison.is_critical:
                # Critical regressions get very low scores
                metric_score = max(0.0, 30.0 + comparison.change_percent)
            elif comparison.is_regression:
                # Regressions get reduced scores
                metric_score = max(50.0, 100.0 + comparison.change_percent)
            elif comparison.change_percent > 0:
                # Improvements get bonus points (capped)
                metric_score = min(110.0, 100.0 + comparison.change_percent * 0.5)
            
            total_score += metric_score
        
        # Average and apply weight
        average_score = total_score / len(comparisons)
        weighted_score = average_score * weight
        
        return min(100.0, max(0.0, weighted_score))
    
    def run_all_regression_tests(self, current_results_file: str) -> List[RegressionResult]:
        """Run all defined regression tests"""
        logger.info("Starting performance regression analysis...")
        
        current_results = self.load_current_results(current_results_file)
        if not current_results:
            logger.error("No current results to analyze")
            return []
        
        self.regression_results = []
        
        for test in self.regression_tests:
            try:
                result = self.analyze_regression_test(test, current_results)
                self.regression_results.append(result)
            except Exception as e:
                logger.error(f"Failed to analyze test {test.name}: {e}")
                # Add failed result
                self.regression_results.append(RegressionResult(
                    test_name=test.name,
                    baseline_name=test.baseline_name,
                    overall_passed=False,
                    performance_score=0.0,
                    metric_comparisons=[],
                    issues_found=[f"Test execution failed: {str(e)}"],
                    recommendations=["Fix test execution issues"],
                    timestamp=0.0
                ))
        
        return self.regression_results
    
    def generate_summary_report(self) -> Dict:
        """Generate a summary report of all regression tests"""
        if not self.regression_results:
            return {'error': 'No regression results available'}
        
        passed_tests = [r for r in self.regression_results if r.overall_passed]
        failed_tests = [r for r in self.regression_results if not r.overall_passed]
        
        # Calculate overall scores
        total_score = mean([r.performance_score for r in self.regression_results])
        
        # Collect all issues
        all_issues = []
        all_recommendations = []
        for result in self.regression_results:
            all_issues.extend(result.issues_found)
            all_recommendations.extend(result.recommendations)
        
        # Critical issues
        critical_issues = [issue for issue in all_issues if 'CRITICAL' in issue]
        regression_issues = [issue for issue in all_issues if 'REGRESSION' in issue]
        
        summary = {
            'overall_status': 'PASSED' if len(critical_issues) == 0 else 'FAILED',
            'overall_score': total_score,
            'tests_run': len(self.regression_results),
            'tests_passed': len(passed_tests),
            'tests_failed': len(failed_tests),
            'critical_issues_count': len(critical_issues),
            'regression_issues_count': len(regression_issues),
            'critical_issues': critical_issues,
            'regression_issues': regression_issues,
            'recommendations': list(set(all_recommendations)),  # Remove duplicates
            'test_results': []
        }
        
        # Add individual test summaries
        for result in self.regression_results:
            summary['test_results'].append({
                'test_name': result.test_name,
                'passed': result.overall_passed,
                'score': result.performance_score,
                'issues_count': len(result.issues_found)
            })
        
        return summary
    
    def save_detailed_report(self, output_file: str):
        """Save detailed regression analysis report"""
        detailed_report = {
            'summary': self.generate_summary_report(),
            'detailed_results': []
        }
        
        for result in self.regression_results:
            detailed_result = {
                'test_name': result.test_name,
                'baseline_name': result.baseline_name,
                'overall_passed': result.overall_passed,
                'performance_score': result.performance_score,
                'timestamp': result.timestamp,
                'issues_found': result.issues_found,
                'recommendations': result.recommendations,
                'metric_comparisons': []
            }
            
            for comparison in result.metric_comparisons:
                detailed_result['metric_comparisons'].append({
                    'metric_name': comparison.metric_name,
                    'baseline_value': comparison.baseline_value,
                    'current_value': comparison.current_value,
                    'change_percent': comparison.change_percent,
                    'is_regression': comparison.is_regression,
                    'is_critical': comparison.is_critical,
                    'threshold_used': comparison.threshold_used
                })
            
            detailed_report['detailed_results'].append(detailed_result)
        
        with open(output_file, 'w') as f:
            json.dump(detailed_report, f, indent=2)
        
        logger.info(f"Detailed regression report saved: {output_file}")
    
    def print_summary(self):
        """Print a human-readable summary to console"""
        if not self.regression_results:
            print("No regression analysis results available")
            return
        
        summary = self.generate_summary_report()
        
        print(f"\n{'='*60}")
        print(f"PERFORMANCE REGRESSION ANALYSIS SUMMARY")
        print(f"{'='*60}")
        print(f"Overall Status: {summary['overall_status']}")
        print(f"Overall Score: {summary['overall_score']:.1f}/100")
        print(f"Tests Run: {summary['tests_run']}")
        print(f"Tests Passed: {summary['tests_passed']}")
        print(f"Tests Failed: {summary['tests_failed']}")
        print(f"Critical Issues: {summary['critical_issues_count']}")
        print(f"Regression Issues: {summary['regression_issues_count']}")
        
        if summary['critical_issues']:
            print(f"\n🚨 CRITICAL ISSUES:")
            for issue in summary['critical_issues']:
                print(f"  - {issue}")
        
        if summary['regression_issues']:
            print(f"\n⚠️  PERFORMANCE REGRESSIONS:")
            for issue in summary['regression_issues']:
                print(f"  - {issue}")
        
        if summary['recommendations']:
            print(f"\n💡 RECOMMENDATIONS:")
            for rec in summary['recommendations']:
                print(f"  - {rec}")
        
        print(f"\n📊 INDIVIDUAL TEST RESULTS:")
        for test in summary['test_results']:
            status = "✅ PASS" if test['passed'] else "❌ FAIL"
            print(f"  {status} {test['test_name']}: {test['score']:.1f}/100")
        
        print(f"{'='*60}")
    
    def should_fail_ci(self, failure_threshold: float = 10.0) -> Tuple[bool, str]:
        """Determine if CI should fail based on regression results"""
        if not self.regression_results:
            return True, "No regression analysis results"
        
        summary = self.generate_summary_report()
        
        # Fail if there are critical issues
        if summary['critical_issues_count'] > 0:
            return True, f"Critical performance issues detected: {summary['critical_issues_count']}"
        
        # Fail if overall score is below threshold
        if summary['overall_score'] < 100 - failure_threshold:
            return True, f"Overall performance score {summary['overall_score']:.1f} below threshold {100-failure_threshold}"
        
        # Fail if too many tests failed
        failure_rate = (summary['tests_failed'] / summary['tests_run']) * 100
        if failure_rate > 50:  # More than half failed
            return True, f"High test failure rate: {failure_rate:.1f}%"
        
        return False, "Performance regression checks passed"

def main():
    parser = argparse.ArgumentParser(description='ClimbingGame Performance Regression Analysis')
    parser.add_argument('--current-results', required=True,
                       help='Path to current benchmark results JSON file')
    parser.add_argument('--baseline-branch', default='main',
                       help='Git branch to compare against (for context)')
    parser.add_argument('--baseline-dir', default='Saved/PerformanceBaselines',
                       help='Directory containing performance baselines')
    parser.add_argument('--output-dir', default='regression-analysis',
                       help='Directory for output files')
    parser.add_argument('--threshold', type=float, default=10.0,
                       help='Failure threshold percentage')
    parser.add_argument('--fail-on-regression', action='store_true',
                       help='Exit with error code if regressions detected')
    
    args = parser.parse_args()
    
    try:
        # Create output directory
        output_dir = Path(args.output_dir)
        output_dir.mkdir(parents=True, exist_ok=True)
        
        # Initialize analyzer
        analyzer = PerformanceRegressionAnalyzer(
            baseline_dir=args.baseline_dir,
            current_results_dir=Path(args.current_results).parent
        )
        
        # Run regression analysis
        results = analyzer.run_all_regression_tests(args.current_results)
        
        if not results:
            logger.error("No regression analysis results generated")
            sys.exit(1)
        
        # Save detailed report
        report_file = output_dir / 'regression_analysis.json'
        analyzer.save_detailed_report(str(report_file))
        
        # Print summary
        analyzer.print_summary()
        
        # Check if CI should fail
        should_fail, reason = analyzer.should_fail_ci(args.threshold)
        
        if should_fail:
            logger.error(f"Performance regression check failed: {reason}")
            if args.fail_on_regression:
                sys.exit(1)
        else:
            logger.info(f"Performance regression check passed: {reason}")
        
        sys.exit(0 if not should_fail else 1)
        
    except Exception as e:
        logger.error(f"Regression analysis failed: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()