#!/usr/bin/env python3
"""
ClimbingGame Performance Dashboard Generator
Creates comprehensive performance monitoring dashboards and reports
"""

import os
import sys
import json
import logging
import argparse
from pathlib import Path
from typing import Dict, List, Optional, Any
from dataclasses import dataclass, asdict
import matplotlib.pyplot as plt
import matplotlib.dates as mdates
import numpy as np
from datetime import datetime, timedelta
import seaborn as sns
import plotly.graph_objects as go
import plotly.express as px
from plotly.subplots import make_subplots
import plotly.offline as pyo
import pandas as pd

# Configure styling
plt.style.use('seaborn-v0_8')
sns.set_palette("husl")

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

@dataclass
class DashboardConfig:
    """Configuration for dashboard generation"""
    title: str = "ClimbingGame Performance Dashboard"
    theme: str = "dark"
    refresh_interval: int = 30  # seconds
    history_days: int = 30
    enable_real_time: bool = True
    include_regression_analysis: bool = True
    include_trend_analysis: bool = True
    generate_static_reports: bool = True

class PerformanceDashboardGenerator:
    """Generates comprehensive performance dashboards and reports"""
    
    def __init__(self, results_dir: str, regression_dir: str = None, output_dir: str = "performance-dashboard"):
        self.results_dir = Path(results_dir)
        self.regression_dir = Path(regression_dir) if regression_dir else None
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(parents=True, exist_ok=True)
        
        # Load performance data
        self.benchmark_results = self.load_benchmark_results()
        self.regression_results = self.load_regression_results()
        self.historical_data = self.load_historical_data()
        
        # Dashboard configuration
        self.config = DashboardConfig()
        
        # Color schemes for different metrics
        self.color_schemes = {
            'fps': '#1f77b4',
            'memory': '#ff7f0e', 
            'cpu': '#2ca02c',
            'physics': '#d62728',
            'network': '#9467bd',
            'loading': '#8c564b'
        }
    
    def load_benchmark_results(self) -> Dict:
        """Load current benchmark results"""
        results_file = self.results_dir / "benchmark_results.json"
        if results_file.exists():
            try:
                with open(results_file, 'r') as f:
                    return json.load(f)
            except Exception as e:
                logger.error(f"Failed to load benchmark results: {e}")
        return {}
    
    def load_regression_results(self) -> Dict:
        """Load regression analysis results"""
        if not self.regression_dir or not self.regression_dir.exists():
            return {}
        
        regression_file = self.regression_dir / "regression_analysis.json"
        if regression_file.exists():
            try:
                with open(regression_file, 'r') as f:
                    return json.load(f)
            except Exception as e:
                logger.error(f"Failed to load regression results: {e}")
        return {}
    
    def load_historical_data(self) -> List[Dict]:
        """Load historical performance data"""
        history_file = self.output_dir / "performance_history.json"
        if history_file.exists():
            try:
                with open(history_file, 'r') as f:
                    return json.load(f)
            except Exception as e:
                logger.error(f"Failed to load historical data: {e}")
        return []
    
    def save_historical_data(self, current_data: Dict):
        """Save current results to historical data"""
        # Add timestamp and append to history
        current_data['analysis_timestamp'] = datetime.now().isoformat()
        self.historical_data.append(current_data)
        
        # Keep only last 30 days of data
        cutoff_date = datetime.now() - timedelta(days=self.config.history_days)
        self.historical_data = [
            data for data in self.historical_data 
            if datetime.fromisoformat(data.get('analysis_timestamp', '1970-01-01')) > cutoff_date
        ]
        
        # Save updated history
        history_file = self.output_dir / "performance_history.json"
        with open(history_file, 'w') as f:
            json.dump(self.historical_data, f, indent=2)
    
    def generate_fps_analysis_chart(self) -> str:
        """Generate FPS performance analysis chart"""
        if not self.benchmark_results.get('scenarios'):
            return ""
        
        fig = make_subplots(
            rows=2, cols=2,
            subplot_titles=('FPS by Scenario', 'FPS Distribution', 'Performance Trends', 'Frame Time Analysis'),
            specs=[[{"secondary_y": False}, {"secondary_y": False}],
                   [{"secondary_y": True}, {"secondary_y": False}]]
        )
        
        # Extract FPS data from scenarios
        scenario_names = []
        fps_means = []
        fps_mins = []
        fps_maxes = []
        
        for scenario in self.benchmark_results.get('scenarios', []):
            if scenario.get('fps'):
                scenario_names.append(scenario['scenario_name'])
                fps_means.append(scenario['fps'].get('mean', 0))
                fps_mins.append(scenario['fps'].get('min', 0))
                fps_maxes.append(scenario['fps'].get('max', 0))
        
        if scenario_names:
            # FPS by Scenario (Bar Chart)
            fig.add_trace(
                go.Bar(name='Mean FPS', x=scenario_names, y=fps_means, 
                      marker_color=self.color_schemes['fps']),
                row=1, col=1
            )
            fig.add_trace(
                go.Bar(name='Min FPS', x=scenario_names, y=fps_mins, 
                      marker_color='rgba(31, 119, 180, 0.6)'),
                row=1, col=1
            )
            
            # FPS Distribution (Histogram)
            all_fps = fps_means + fps_mins + fps_maxes
            fig.add_trace(
                go.Histogram(x=all_fps, name='FPS Distribution',
                           marker_color=self.color_schemes['fps']),
                row=1, col=2
            )
        
        # Historical trends (if available)
        if self.historical_data:
            dates = []
            historical_fps = []
            
            for data in self.historical_data:
                if data.get('analysis_timestamp') and data.get('scenarios'):
                    dates.append(datetime.fromisoformat(data['analysis_timestamp']))
                    scenario_fps = []
                    for scenario in data.get('scenarios', []):
                        if scenario.get('fps', {}).get('mean'):
                            scenario_fps.append(scenario['fps']['mean'])
                    historical_fps.append(np.mean(scenario_fps) if scenario_fps else 0)
            
            if dates:
                fig.add_trace(
                    go.Scatter(x=dates, y=historical_fps, mode='lines+markers',
                             name='Historical FPS Trend', 
                             line=dict(color=self.color_schemes['fps'])),
                    row=2, col=1
                )
        
        # Frame Time Analysis
        if scenario_names:
            frame_times = [1000.0 / fps if fps > 0 else 0 for fps in fps_means]
            fig.add_trace(
                go.Scatter(x=scenario_names, y=frame_times, mode='lines+markers',
                         name='Frame Time (ms)', 
                         line=dict(color=self.color_schemes['physics'])),
                row=2, col=2
            )
        
        fig.update_layout(
            title="FPS Performance Analysis",
            height=800,
            showlegend=True,
            template="plotly_dark" if self.config.theme == "dark" else "plotly_white"
        )
        
        # Save chart
        chart_file = self.output_dir / "fps_analysis.html"
        fig.write_html(str(chart_file))
        return str(chart_file)
    
    def generate_memory_analysis_chart(self) -> str:
        """Generate memory usage analysis chart"""
        if not self.benchmark_results.get('scenarios'):
            return ""
        
        fig = make_subplots(
            rows=2, cols=2,
            subplot_titles=('Memory Usage by Scenario', 'Memory Growth', 'Memory Efficiency', 'Memory Timeline'),
            specs=[[{"secondary_y": False}, {"secondary_y": False}],
                   [{"secondary_y": False}, {"secondary_y": False}]]
        )
        
        # Extract memory data
        scenario_names = []
        memory_means = []
        memory_maxes = []
        memory_growth = []
        
        for scenario in self.benchmark_results.get('scenarios', []):
            if scenario.get('memory_usage'):
                scenario_names.append(scenario['scenario_name'])
                memory_means.append(scenario['memory_usage'].get('mean', 0))
                memory_maxes.append(scenario['memory_usage'].get('max', 0))
                memory_growth.append(scenario['memory_usage'].get('growth', 0))
        
        if scenario_names:
            # Memory Usage by Scenario
            fig.add_trace(
                go.Bar(name='Peak Memory (MB)', x=scenario_names, y=memory_maxes,
                      marker_color=self.color_schemes['memory']),
                row=1, col=1
            )
            fig.add_trace(
                go.Bar(name='Average Memory (MB)', x=scenario_names, y=memory_means,
                      marker_color='rgba(255, 127, 14, 0.6)'),
                row=1, col=1
            )
            
            # Memory Growth
            fig.add_trace(
                go.Bar(name='Memory Growth (MB)', x=scenario_names, y=memory_growth,
                      marker_color='rgba(255, 127, 14, 0.8)'),
                row=1, col=2
            )
            
            # Memory Efficiency (Memory per FPS)
            fps_means = []
            for scenario in self.benchmark_results.get('scenarios', []):
                if scenario.get('fps'):
                    fps_means.append(scenario['fps'].get('mean', 1))
                else:
                    fps_means.append(1)
            
            efficiency = [mem / fps if fps > 0 else 0 for mem, fps in zip(memory_means, fps_means)]
            fig.add_trace(
                go.Bar(name='Memory/FPS Ratio', x=scenario_names, y=efficiency,
                      marker_color='rgba(255, 127, 14, 0.4)'),
                row=2, col=1
            )
        
        # Historical memory trends
        if self.historical_data:
            dates = []
            historical_memory = []
            
            for data in self.historical_data:
                if data.get('analysis_timestamp') and data.get('scenarios'):
                    dates.append(datetime.fromisoformat(data['analysis_timestamp']))
                    scenario_memory = []
                    for scenario in data.get('scenarios', []):
                        if scenario.get('memory_usage', {}).get('max'):
                            scenario_memory.append(scenario['memory_usage']['max'])
                    historical_memory.append(np.mean(scenario_memory) if scenario_memory else 0)
            
            if dates:
                fig.add_trace(
                    go.Scatter(x=dates, y=historical_memory, mode='lines+markers',
                             name='Historical Memory Trend',
                             line=dict(color=self.color_schemes['memory'])),
                    row=2, col=2
                )
        
        fig.update_layout(
            title="Memory Usage Analysis", 
            height=800,
            showlegend=True,
            template="plotly_dark" if self.config.theme == "dark" else "plotly_white"
        )
        
        chart_file = self.output_dir / "memory_analysis.html"
        fig.write_html(str(chart_file))
        return str(chart_file)
    
    def generate_regression_analysis_chart(self) -> str:
        """Generate regression analysis visualization"""
        if not self.regression_results or not self.regression_results.get('detailed_results'):
            return ""
        
        fig = make_subplots(
            rows=2, cols=2,
            subplot_titles=('Test Results Overview', 'Performance Scores', 'Regression Severity', 'Metric Changes'),
            specs=[[{"type": "pie"}, {"secondary_y": False}],
                   [{"secondary_y": False}, {"secondary_y": False}]]
        )
        
        summary = self.regression_results.get('summary', {})
        detailed_results = self.regression_results.get('detailed_results', [])
        
        # Test Results Overview (Pie Chart)
        if summary:
            labels = ['Passed', 'Failed']
            values = [summary.get('tests_passed', 0), summary.get('tests_failed', 0)]
            colors = ['#2ca02c', '#d62728']
            
            fig.add_trace(
                go.Pie(labels=labels, values=values, marker_colors=colors,
                      name="Test Results"),
                row=1, col=1
            )
        
        # Performance Scores
        if detailed_results:
            test_names = [result.get('test_name', 'Unknown') for result in detailed_results]
            scores = [result.get('performance_score', 0) for result in detailed_results]
            colors = ['#2ca02c' if score >= 70 else '#ff7f0e' if score >= 50 else '#d62728' 
                     for score in scores]
            
            fig.add_trace(
                go.Bar(name='Performance Score', x=test_names, y=scores,
                      marker_color=colors),
                row=1, col=2
            )
            
            # Add baseline at 70 (passing threshold)
            fig.add_hline(y=70, line_dash="dash", line_color="white", 
                         annotation_text="Passing Threshold", row=1, col=2)
        
        # Regression Severity
        if detailed_results:
            regression_counts = {'Critical': 0, 'Regression': 0, 'Warning': 0, 'Pass': 0}
            
            for result in detailed_results:
                issues = result.get('issues_found', [])
                if any('CRITICAL' in issue for issue in issues):
                    regression_counts['Critical'] += 1
                elif any('REGRESSION' in issue for issue in issues):
                    regression_counts['Regression'] += 1
                elif issues:
                    regression_counts['Warning'] += 1
                else:
                    regression_counts['Pass'] += 1
            
            fig.add_trace(
                go.Bar(name='Issue Severity', 
                      x=list(regression_counts.keys()), 
                      y=list(regression_counts.values()),
                      marker_color=['#d62728', '#ff7f0e', '#1f77b4', '#2ca02c']),
                row=2, col=1
            )
        
        # Metric Changes
        if detailed_results:
            all_changes = []
            all_metrics = []
            all_tests = []
            
            for result in detailed_results:
                for comparison in result.get('metric_comparisons', []):
                    all_changes.append(comparison.get('change_percent', 0))
                    all_metrics.append(comparison.get('metric_name', 'Unknown'))
                    all_tests.append(result.get('test_name', 'Unknown'))
            
            if all_changes:
                fig.add_trace(
                    go.Scatter(x=all_metrics, y=all_changes, mode='markers',
                             marker=dict(
                                 size=8,
                                 color=all_changes,
                                 colorscale='RdYlGn_r',
                                 showscale=True,
                                 colorbar=dict(title="Change %")
                             ),
                             text=all_tests,
                             name='Metric Changes'),
                    row=2, col=2
                )
                
                # Add zero line
                fig.add_hline(y=0, line_dash="dash", line_color="white", 
                             annotation_text="No Change", row=2, col=2)
        
        fig.update_layout(
            title="Performance Regression Analysis",
            height=800,
            showlegend=False,
            template="plotly_dark" if self.config.theme == "dark" else "plotly_white"
        )
        
        chart_file = self.output_dir / "regression_analysis.html"
        fig.write_html(str(chart_file))
        return str(chart_file)
    
    def generate_system_overview_chart(self) -> str:
        """Generate system performance overview"""
        fig = make_subplots(
            rows=2, cols=3,
            subplot_titles=('Overall Performance Score', 'Resource Utilization', 'Performance Trends',
                          'Scenario Comparison', 'System Health', 'Bottleneck Analysis'),
            specs=[[{"type": "indicator"}, {"type": "bar"}, {"secondary_y": False}],
                   [{"secondary_y": False}, {"type": "indicator"}, {"type": "bar"}]]
        )
        
        # Calculate overall performance score
        overall_score = 0
        if self.regression_results.get('summary'):
            overall_score = self.regression_results['summary'].get('overall_score', 0)
        elif self.benchmark_results.get('scenarios'):
            # Calculate from benchmark results
            scores = []
            for scenario in self.benchmark_results['scenarios']:
                if scenario.get('performance_score'):
                    scores.append(scenario['performance_score'])
            overall_score = np.mean(scores) if scores else 0
        
        # Overall Performance Score (Gauge)
        fig.add_trace(
            go.Indicator(
                mode="gauge+number+delta",
                value=overall_score,
                delta={'reference': 85, 'increasing': {'color': "green"}, 'decreasing': {'color': "red"}},
                gauge={
                    'axis': {'range': [None, 100]},
                    'bar': {'color': "darkblue"},
                    'steps': [
                        {'range': [0, 50], 'color': "lightgray"},
                        {'range': [50, 85], 'color': "gray"}
                    ],
                    'threshold': {
                        'line': {'color': "red", 'width': 4},
                        'thickness': 0.75,
                        'value': 90
                    }
                },
                title={'text': "Overall Score"}
            ),
            row=1, col=1
        )
        
        # Resource Utilization
        if self.benchmark_results.get('scenarios'):
            resource_types = ['CPU', 'Memory', 'GPU', 'Network']
            # These would be calculated from actual data
            utilization = [65, 78, 45, 23]  # Placeholder values
            
            fig.add_trace(
                go.Bar(name='Resource Utilization %', x=resource_types, y=utilization,
                      marker_color=['#1f77b4', '#ff7f0e', '#2ca02c', '#d62728']),
                row=1, col=2
            )
        
        # Performance Trends
        if self.historical_data:
            dates = [datetime.fromisoformat(data['analysis_timestamp']) 
                    for data in self.historical_data if data.get('analysis_timestamp')]
            
            if dates:
                # Extract performance metrics over time
                fps_trend = []
                memory_trend = []
                
                for data in self.historical_data:
                    scenarios = data.get('scenarios', [])
                    if scenarios:
                        fps_vals = [s.get('fps', {}).get('mean', 0) for s in scenarios]
                        mem_vals = [s.get('memory_usage', {}).get('max', 0) for s in scenarios]
                        fps_trend.append(np.mean([f for f in fps_vals if f > 0]) if fps_vals else 0)
                        memory_trend.append(np.mean([m for m in mem_vals if m > 0]) if mem_vals else 0)
                
                fig.add_trace(
                    go.Scatter(x=dates, y=fps_trend, mode='lines+markers',
                             name='FPS Trend', line=dict(color='#1f77b4')),
                    row=1, col=3
                )
        
        # Scenario Comparison
        if self.benchmark_results.get('scenarios'):
            scenario_names = [s.get('scenario_name', 'Unknown')[:15] + '...' 
                             if len(s.get('scenario_name', '')) > 15 
                             else s.get('scenario_name', 'Unknown')
                             for s in self.benchmark_results['scenarios']]
            scenario_scores = [s.get('performance_score', 0) for s in self.benchmark_results['scenarios']]
            
            fig.add_trace(
                go.Bar(name='Scenario Performance', x=scenario_names, y=scenario_scores,
                      marker_color=['#2ca02c' if score >= 80 else '#ff7f0e' if score >= 60 else '#d62728' 
                                   for score in scenario_scores]),
                row=2, col=1
            )
        
        # System Health Indicator
        health_score = min(100, overall_score * 1.1)  # Boost health score slightly
        health_color = "green" if health_score >= 80 else "orange" if health_score >= 60 else "red"
        
        fig.add_trace(
            go.Indicator(
                mode="number+gauge",
                value=health_score,
                gauge={
                    'shape': "bullet",
                    'axis': {'range': [None, 100]},
                    'threshold': {
                        'line': {'color': "red", 'width': 2},
                        'thickness': 0.75,
                        'value': 85
                    },
                    'bar': {'color': health_color}
                },
                title={'text': "System Health"}
            ),
            row=2, col=2
        )
        
        # Bottleneck Analysis
        bottlenecks = ['Physics', 'Rendering', 'Memory', 'Network', 'I/O']
        bottleneck_severity = [85, 45, 60, 30, 25]  # Placeholder - would be calculated from actual data
        
        fig.add_trace(
            go.Bar(name='Bottleneck Severity', x=bottlenecks, y=bottleneck_severity,
                  marker_color=['#d62728' if sev >= 80 else '#ff7f0e' if sev >= 60 else '#2ca02c' 
                               for sev in bottleneck_severity]),
            row=2, col=3
        )
        
        fig.update_layout(
            title="ClimbingGame Performance Dashboard - System Overview",
            height=900,
            showlegend=False,
            template="plotly_dark" if self.config.theme == "dark" else "plotly_white"
        )
        
        chart_file = self.output_dir / "system_overview.html"
        fig.write_html(str(chart_file))
        return str(chart_file)
    
    def generate_main_dashboard(self) -> str:
        """Generate the main dashboard HTML page"""
        dashboard_html = f"""
        <!DOCTYPE html>
        <html lang="en">
        <head>
            <meta charset="UTF-8">
            <meta name="viewport" content="width=device-width, initial-scale=1.0">
            <title>{self.config.title}</title>
            <style>
                body {{
                    font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
                    margin: 0;
                    padding: 20px;
                    background-color: {'#1e1e1e' if self.config.theme == 'dark' else '#ffffff'};
                    color: {'#ffffff' if self.config.theme == 'dark' else '#000000'};
                }}
                .header {{
                    text-align: center;
                    margin-bottom: 30px;
                    padding: 20px;
                    background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
                    color: white;
                    border-radius: 10px;
                }}
                .dashboard-grid {{
                    display: grid;
                    grid-template-columns: repeat(auto-fit, minmax(500px, 1fr));
                    gap: 20px;
                    margin-bottom: 30px;
                }}
                .chart-container {{
                    background-color: {'#2d2d2d' if self.config.theme == 'dark' else '#f8f9fa'};
                    border-radius: 10px;
                    padding: 15px;
                    box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);
                }}
                .chart-container iframe {{
                    width: 100%;
                    height: 600px;
                    border: none;
                    border-radius: 5px;
                }}
                .status-bar {{
                    display: flex;
                    justify-content: space-between;
                    align-items: center;
                    padding: 15px;
                    background-color: {'#333' if self.config.theme == 'dark' else '#e9ecef'};
                    border-radius: 5px;
                    margin-bottom: 20px;
                }}
                .status-item {{
                    text-align: center;
                }}
                .status-value {{
                    font-size: 24px;
                    font-weight: bold;
                    color: #007bff;
                }}
                .status-label {{
                    font-size: 12px;
                    opacity: 0.7;
                }}
                .alert {{
                    padding: 15px;
                    margin-bottom: 20px;
                    border-radius: 5px;
                    border: 1px solid transparent;
                }}
                .alert-success {{ background-color: #d4edda; border-color: #c3e6cb; color: #155724; }}
                .alert-warning {{ background-color: #fff3cd; border-color: #ffeaa7; color: #856404; }}
                .alert-danger {{ background-color: #f8d7da; border-color: #f5c6cb; color: #721c24; }}
                .footer {{
                    text-align: center;
                    margin-top: 40px;
                    padding: 20px;
                    opacity: 0.6;
                    font-size: 12px;
                }}
                @media (max-width: 768px) {{
                    .dashboard-grid {{
                        grid-template-columns: 1fr;
                    }}
                    .status-bar {{
                        flex-direction: column;
                        gap: 10px;
                    }}
                }}
            </style>
            {f'<meta http-equiv="refresh" content="{self.config.refresh_interval}">' if self.config.enable_real_time else ''}
        </head>
        <body>
            <div class="header">
                <h1>{self.config.title}</h1>
                <p>Real-time performance monitoring and analysis for ClimbingGame</p>
                <p>Last Updated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S UTC')}</p>
            </div>
        """
        
        # Add status bar
        if self.regression_results.get('summary'):
            summary = self.regression_results['summary']
            status_class = 'alert-success' if summary.get('overall_status') == 'PASSED' else 'alert-danger'
            
            dashboard_html += f"""
            <div class="alert {status_class}">
                <strong>Overall Status:</strong> {summary.get('overall_status', 'Unknown')} | 
                <strong>Performance Score:</strong> {summary.get('overall_score', 0):.1f}/100 |
                <strong>Tests:</strong> {summary.get('tests_passed', 0)}/{summary.get('tests_run', 0)} Passed |
                <strong>Issues:</strong> {summary.get('critical_issues_count', 0)} Critical, {summary.get('regression_issues_count', 0)} Regressions
            </div>
            """
        
        # Add status metrics bar
        dashboard_html += """
            <div class="status-bar">
                <div class="status-item">
                    <div class="status-value">""" + str(len(self.benchmark_results.get('scenarios', []))) + """</div>
                    <div class="status-label">Scenarios Tested</div>
                </div>
                <div class="status-item">
                    <div class="status-value">""" + str(len(self.historical_data)) + """</div>
                    <div class="status-label">Historical Records</div>
                </div>
                <div class="status-item">
                    <div class="status-value">""" + ('Yes' if self.config.enable_real_time else 'No') + """</div>
                    <div class="status-label">Real-time Updates</div>
                </div>
                <div class="status-item">
                    <div class="status-value">""" + str(self.config.refresh_interval) + """s</div>
                    <div class="status-label">Refresh Interval</div>
                </div>
            </div>
        """
        
        # Add chart containers
        dashboard_html += """
            <div class="dashboard-grid">
                <div class="chart-container">
                    <h3>System Overview</h3>
                    <iframe src="system_overview.html"></iframe>
                </div>
                <div class="chart-container">
                    <h3>FPS Performance Analysis</h3>
                    <iframe src="fps_analysis.html"></iframe>
                </div>
                <div class="chart-container">
                    <h3>Memory Usage Analysis</h3>
                    <iframe src="memory_analysis.html"></iframe>
                </div>
        """
        
        if self.regression_results:
            dashboard_html += """
                <div class="chart-container">
                    <h3>Regression Analysis</h3>
                    <iframe src="regression_analysis.html"></iframe>
                </div>
            """
        
        dashboard_html += """
            </div>
            <div class="footer">
                <p>ClimbingGame Performance Dashboard | Generated by Automated Performance Monitoring System</p>
                <p>For technical support or questions, please contact the development team.</p>
            </div>
        </body>
        </html>
        """
        
        # Save dashboard
        dashboard_file = self.output_dir / "index.html"
        with open(dashboard_file, 'w') as f:
            f.write(dashboard_html)
        
        return str(dashboard_file)
    
    def generate_all_dashboards(self) -> List[str]:
        """Generate all dashboard components"""
        logger.info("Generating performance dashboards...")
        
        generated_files = []
        
        try:
            # Generate individual charts
            generated_files.append(self.generate_system_overview_chart())
            generated_files.append(self.generate_fps_analysis_chart())
            generated_files.append(self.generate_memory_analysis_chart())
            
            if self.regression_results:
                generated_files.append(self.generate_regression_analysis_chart())
            
            # Generate main dashboard
            main_dashboard = self.generate_main_dashboard()
            generated_files.append(main_dashboard)
            
            # Save current results to historical data
            if self.benchmark_results:
                self.save_historical_data(self.benchmark_results)
            
            logger.info(f"Generated {len(generated_files)} dashboard files")
            
        except Exception as e:
            logger.error(f"Failed to generate dashboards: {e}")
            raise
        
        return [f for f in generated_files if f]  # Filter out empty strings
    
    def generate_static_report(self) -> str:
        """Generate a static PDF/HTML report"""
        # This would generate a comprehensive static report
        # For now, return the main dashboard
        return self.generate_main_dashboard()

def main():
    parser = argparse.ArgumentParser(description='Generate ClimbingGame Performance Dashboard')
    parser.add_argument('--results-dir', required=True,
                       help='Directory containing benchmark results')
    parser.add_argument('--regression-dir',
                       help='Directory containing regression analysis results')
    parser.add_argument('--output-dir', default='performance-dashboard',
                       help='Output directory for dashboard files')
    parser.add_argument('--theme', choices=['light', 'dark'], default='dark',
                       help='Dashboard theme')
    parser.add_argument('--refresh-interval', type=int, default=30,
                       help='Auto-refresh interval in seconds')
    parser.add_argument('--no-realtime', action='store_true',
                       help='Disable real-time updates')
    
    args = parser.parse_args()
    
    try:
        # Initialize dashboard generator
        generator = PerformanceDashboardGenerator(
            results_dir=args.results_dir,
            regression_dir=args.regression_dir,
            output_dir=args.output_dir
        )
        
        # Configure dashboard
        generator.config.theme = args.theme
        generator.config.refresh_interval = args.refresh_interval
        generator.config.enable_real_time = not args.no_realtime
        
        # Generate dashboards
        generated_files = generator.generate_all_dashboards()
        
        print(f"\n🎯 Performance Dashboard Generated Successfully!")
        print(f"📊 Dashboard Location: {Path(args.output_dir).resolve()}")
        print(f"🌐 Main Dashboard: {Path(args.output_dir).resolve() / 'index.html'}")
        print(f"📈 Generated {len(generated_files)} dashboard components")
        
        if generator.config.enable_real_time:
            print(f"🔄 Real-time updates enabled (refresh every {args.refresh_interval}s)")
        
        sys.exit(0)
        
    except Exception as e:
        logger.error(f"Dashboard generation failed: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()