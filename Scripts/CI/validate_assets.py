#!/usr/bin/env python3
"""
ClimbingGame Asset Validation Pipeline
Validates and optimizes game assets for performance and consistency
"""

import os
import sys
import json
import logging
import argparse
from pathlib import Path
from typing import Dict, List, Tuple, Optional
import hashlib
import subprocess
from PIL import Image
import re

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

class AssetValidator:
    """Asset validation and optimization system for ClimbingGame"""
    
    def __init__(self, project_dir: str):
        self.project_dir = Path(project_dir).resolve()
        self.content_dir = self.project_dir / "Content"
        
        # Asset validation rules
        self.texture_rules = {
            'max_resolution': (4096, 4096),
            'min_resolution': (64, 64),
            'supported_formats': ['.png', '.jpg', '.jpeg', '.tga', '.bmp', '.dds'],
            'power_of_two_required': True,
            'max_file_size_mb': 50
        }
        
        self.mesh_rules = {
            'supported_formats': ['.fbx', '.obj', '.dae', '.3ds'],
            'max_file_size_mb': 100,
            'max_polygons': 100000
        }
        
        self.audio_rules = {
            'supported_formats': ['.wav', '.ogg', '.mp3'],
            'max_file_size_mb': 25,
            'sample_rates': [22050, 44100, 48000],
            'bit_depths': [16, 24]
        }
        
        # Asset naming conventions
        self.naming_patterns = {
            'textures': {
                'diffuse': r'.*_D\.(png|jpg|tga)$',
                'normal': r'.*_N\.(png|jpg|tga)$',
                'roughness': r'.*_R\.(png|jpg|tga)$',
                'metallic': r'.*_M\.(png|jpg|tga)$',
                'ambient_occlusion': r'.*_AO\.(png|jpg|tga)$'
            },
            'meshes': {
                'static_mesh': r'SM_.*\.(fbx|obj)$',
                'skeletal_mesh': r'SK_.*\.fbx$'
            },
            'blueprints': {
                'actor_blueprint': r'BP_.*\.uasset$',
                'widget_blueprint': r'WBP_.*\.uasset$'
            }
        }
        
        self.validation_results = {
            'errors': [],
            'warnings': [],
            'info': [],
            'stats': {}
        }
    
    def validate_texture(self, texture_path: Path) -> List[Dict]:
        """Validate individual texture file"""
        issues = []
        
        try:
            # Check file size
            file_size_mb = texture_path.stat().st_size / (1024 * 1024)
            if file_size_mb > self.texture_rules['max_file_size_mb']:
                issues.append({
                    'type': 'error',
                    'file': str(texture_path),
                    'message': f"Texture file size ({file_size_mb:.2f}MB) exceeds limit ({self.texture_rules['max_file_size_mb']}MB)"
                })
            
            # Check format
            if texture_path.suffix.lower() not in self.texture_rules['supported_formats']:
                issues.append({
                    'type': 'warning',
                    'file': str(texture_path),
                    'message': f"Unsupported texture format: {texture_path.suffix}"
                })
                return issues
            
            # Check image properties
            with Image.open(texture_path) as img:
                width, height = img.size
                
                # Check resolution limits
                max_w, max_h = self.texture_rules['max_resolution']
                min_w, min_h = self.texture_rules['min_resolution']
                
                if width > max_w or height > max_h:
                    issues.append({
                        'type': 'error',
                        'file': str(texture_path),
                        'message': f"Texture resolution ({width}x{height}) exceeds maximum ({max_w}x{max_h})"
                    })
                
                if width < min_w or height < min_h:
                    issues.append({
                        'type': 'warning',
                        'file': str(texture_path),
                        'message': f"Texture resolution ({width}x{height}) below minimum ({min_w}x{min_h})"
                    })
                
                # Check power of two
                if self.texture_rules['power_of_two_required']:
                    if not (width & (width - 1) == 0) or not (height & (height - 1) == 0):
                        issues.append({
                            'type': 'warning',
                            'file': str(texture_path),
                            'message': f"Texture resolution ({width}x{height}) is not power of two"
                        })
                
                # Check aspect ratio for UI textures
                if 'UI' in str(texture_path) and width != height:
                    issues.append({
                        'type': 'info',
                        'file': str(texture_path),
                        'message': f"UI texture has non-square aspect ratio ({width}x{height})"
                    })
        
        except Exception as e:
            issues.append({
                'type': 'error',
                'file': str(texture_path),
                'message': f"Failed to validate texture: {e}"
            })
        
        return issues
    
    def validate_mesh(self, mesh_path: Path) -> List[Dict]:
        """Validate 3D mesh file"""
        issues = []
        
        # Check file size
        file_size_mb = mesh_path.stat().st_size / (1024 * 1024)
        if file_size_mb > self.mesh_rules['max_file_size_mb']:
            issues.append({
                'type': 'error',
                'file': str(mesh_path),
                'message': f"Mesh file size ({file_size_mb:.2f}MB) exceeds limit ({self.mesh_rules['max_file_size_mb']}MB)"
            })
        
        # Check format
        if mesh_path.suffix.lower() not in self.mesh_rules['supported_formats']:
            issues.append({
                'type': 'warning',
                'file': str(mesh_path),
                'message': f"Unsupported mesh format: {mesh_path.suffix}"
            })
        
        # Additional mesh validation would require FBX SDK or similar
        # For now, we'll do basic file validation
        
        return issues
    
    def validate_audio(self, audio_path: Path) -> List[Dict]:
        """Validate audio file"""
        issues = []
        
        # Check file size
        file_size_mb = audio_path.stat().st_size / (1024 * 1024)
        if file_size_mb > self.audio_rules['max_file_size_mb']:
            issues.append({
                'type': 'warning',
                'file': str(audio_path),
                'message': f"Audio file size ({file_size_mb:.2f}MB) exceeds recommended limit ({self.audio_rules['max_file_size_mb']}MB)"
            })
        
        # Check format
        if audio_path.suffix.lower() not in self.audio_rules['supported_formats']:
            issues.append({
                'type': 'warning',
                'file': str(audio_path),
                'message': f"Unsupported audio format: {audio_path.suffix}"
            })
        
        return issues
    
    def validate_naming_conventions(self, file_path: Path) -> List[Dict]:
        """Validate file naming conventions"""
        issues = []
        relative_path = file_path.relative_to(self.content_dir)
        
        # Check for spaces in filenames
        if ' ' in file_path.name:
            issues.append({
                'type': 'warning',
                'file': str(file_path),
                'message': "Filename contains spaces, consider using underscores"
            })
        
        # Check for special characters
        if re.search(r'[^a-zA-Z0-9._-]', file_path.stem):
            issues.append({
                'type': 'warning',
                'file': str(file_path),
                'message': "Filename contains special characters"
            })
        
        # Check specific naming patterns
        file_type = None
        if file_path.suffix.lower() in ['.png', '.jpg', '.jpeg', '.tga', '.bmp']:
            file_type = 'texture'
        elif file_path.suffix.lower() in ['.fbx', '.obj', '.dae']:
            file_type = 'mesh'
        elif file_path.suffix.lower() == '.uasset':
            if file_path.stem.startswith('BP_'):
                file_type = 'blueprint'
        
        # Additional naming convention checks can be added here
        
        return issues
    
    def check_duplicate_assets(self) -> List[Dict]:
        """Find potential duplicate assets based on file hash"""
        issues = []
        file_hashes = {}
        
        logger.info("Checking for duplicate assets...")
        
        for file_path in self.content_dir.rglob('*'):
            if file_path.is_file() and file_path.suffix.lower() in ['.png', '.jpg', '.jpeg', '.fbx', '.wav', '.ogg']:
                try:
                    with open(file_path, 'rb') as f:
                        file_hash = hashlib.md5(f.read()).hexdigest()
                    
                    if file_hash in file_hashes:
                        issues.append({
                            'type': 'warning',
                            'file': str(file_path),
                            'message': f"Potential duplicate of: {file_hashes[file_hash]}"
                        })
                    else:
                        file_hashes[file_hash] = str(file_path)
                
                except Exception as e:
                    logger.warning(f"Could not hash file {file_path}: {e}")
        
        return issues
    
    def validate_project_structure(self) -> List[Dict]:
        """Validate project folder structure and organization"""
        issues = []
        
        # Required directories
        required_dirs = [
            'Blueprints',
            'Materials',
            'Meshes',
            'Textures',
            'Audio',
            'Maps'
        ]
        
        for req_dir in required_dirs:
            dir_path = self.content_dir / req_dir
            if not dir_path.exists():
                issues.append({
                    'type': 'warning',
                    'file': str(self.content_dir),
                    'message': f"Missing recommended directory: {req_dir}"
                })
        
        # Check for files in root Content directory
        root_files = [f for f in self.content_dir.iterdir() if f.is_file()]
        if root_files:
            issues.append({
                'type': 'info',
                'file': str(self.content_dir),
                'message': f"Found {len(root_files)} files in root Content directory, consider organizing into subdirectories"
            })
        
        return issues
    
    def generate_asset_report(self) -> Dict:
        """Generate comprehensive asset statistics report"""
        stats = {
            'total_files': 0,
            'total_size_mb': 0,
            'file_types': {},
            'largest_files': [],
            'directory_sizes': {}
        }
        
        all_files = []
        
        for file_path in self.content_dir.rglob('*'):
            if file_path.is_file():
                file_size = file_path.stat().st_size
                file_size_mb = file_size / (1024 * 1024)
                
                stats['total_files'] += 1
                stats['total_size_mb'] += file_size_mb
                
                # Track by file type
                ext = file_path.suffix.lower()
                if ext not in stats['file_types']:
                    stats['file_types'][ext] = {'count': 0, 'size_mb': 0}
                
                stats['file_types'][ext]['count'] += 1
                stats['file_types'][ext]['size_mb'] += file_size_mb
                
                # Track largest files
                all_files.append((str(file_path), file_size_mb))
        
        # Get top 10 largest files
        all_files.sort(key=lambda x: x[1], reverse=True)
        stats['largest_files'] = all_files[:10]
        
        return stats
    
    def run_validation(self) -> Dict:
        """Run complete asset validation pipeline"""
        logger.info("Starting asset validation pipeline...")
        
        # Reset results
        self.validation_results = {
            'errors': [],
            'warnings': [],
            'info': [],
            'stats': {}
        }
        
        # Validate project structure
        structure_issues = self.validate_project_structure()
        for issue in structure_issues:
            self.validation_results[issue['type']].append(issue)
        
        # Check for duplicates
        duplicate_issues = self.check_duplicate_assets()
        for issue in duplicate_issues:
            self.validation_results[issue['type']].append(issue)
        
        # Validate individual assets
        asset_count = 0
        
        # Validate textures
        for texture_path in self.content_dir.rglob('*'):
            if texture_path.is_file() and texture_path.suffix.lower() in ['.png', '.jpg', '.jpeg', '.tga', '.bmp', '.dds']:
                asset_count += 1
                
                # Naming convention validation
                naming_issues = self.validate_naming_conventions(texture_path)
                for issue in naming_issues:
                    self.validation_results[issue['type']].append(issue)
                
                # Texture-specific validation
                texture_issues = self.validate_texture(texture_path)
                for issue in texture_issues:
                    self.validation_results[issue['type']].append(issue)
        
        # Validate meshes
        for mesh_path in self.content_dir.rglob('*'):
            if mesh_path.is_file() and mesh_path.suffix.lower() in ['.fbx', '.obj', '.dae', '.3ds']:
                asset_count += 1
                
                naming_issues = self.validate_naming_conventions(mesh_path)
                for issue in naming_issues:
                    self.validation_results[issue['type']].append(issue)
                
                mesh_issues = self.validate_mesh(mesh_path)
                for issue in mesh_issues:
                    self.validation_results[issue['type']].append(issue)
        
        # Validate audio
        for audio_path in self.content_dir.rglob('*'):
            if audio_path.is_file() and audio_path.suffix.lower() in ['.wav', '.ogg', '.mp3']:
                asset_count += 1
                
                naming_issues = self.validate_naming_conventions(audio_path)
                for issue in naming_issues:
                    self.validation_results[issue['type']].append(issue)
                
                audio_issues = self.validate_audio(audio_path)
                for issue in audio_issues:
                    self.validation_results[issue['type']].append(issue)
        
        # Generate statistics
        self.validation_results['stats'] = self.generate_asset_report()
        self.validation_results['stats']['validated_assets'] = asset_count
        
        logger.info(f"Validation completed for {asset_count} assets")
        logger.info(f"Found {len(self.validation_results['errors'])} errors, {len(self.validation_results['warnings'])} warnings")
        
        return self.validation_results
    
    def save_report(self, output_file: str):
        """Save validation report to JSON file"""
        with open(output_file, 'w') as f:
            json.dump(self.validation_results, f, indent=2)
        
        logger.info(f"Validation report saved to: {output_file}")

def main():
    parser = argparse.ArgumentParser(description='ClimbingGame Asset Validation Pipeline')
    parser.add_argument('--project-dir', default='.', 
                       help='Path to the project directory')
    parser.add_argument('--report-file', default='asset-validation-report.json',
                       help='Output file for validation report')
    parser.add_argument('--fail-on-errors', action='store_true',
                       help='Exit with error code if validation errors found')
    
    args = parser.parse_args()
    
    try:
        validator = AssetValidator(args.project_dir)
        results = validator.run_validation()
        
        # Save report
        validator.save_report(args.report_file)
        
        # Print summary
        print(f"\nAsset Validation Summary:")
        print(f"  Total Assets Validated: {results['stats'].get('validated_assets', 0)}")
        print(f"  Errors: {len(results['errors'])}")
        print(f"  Warnings: {len(results['warnings'])}")
        print(f"  Info: {len(results['info'])}")
        print(f"  Total Project Size: {results['stats'].get('total_size_mb', 0):.2f} MB")
        
        # Exit with appropriate code
        if args.fail_on_errors and results['errors']:
            sys.exit(1)
        else:
            sys.exit(0)
    
    except Exception as e:
        logger.error(f"Asset validation failed: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()