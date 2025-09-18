#!/usr/bin/env python3
"""
generate_geometry.py

Wrapper script to generate geometry data and save to src/data.h
Usage:
  python3 generate_geometry.py sphere   # generates sphere data
  python3 generate_geometry.py cube     # generates cube data
"""

import sys
import subprocess
import os

def main():
    if len(sys.argv) != 2:
        print("Usage: python3 generate_geometry.py [sphere|cube]")
        sys.exit(1)
    
    geometry_type = sys.argv[1].lower()
    
    if geometry_type == "sphere":
        script = "gen_sphere.py"
    elif geometry_type == "cube":
        script = "gen_cube.py"
    else:
        print("Error: geometry type must be 'sphere' or 'cube'")
        sys.exit(1)
    
    # Run the generator script and save output to src/data.h
    try:
        with open("src/data.h", "w") as f:
            subprocess.run([sys.executable, script], stdout=f, check=True)
        print(f"Generated {geometry_type} data -> src/data.h")
    except subprocess.CalledProcessError as e:
        print(f"Error running {script}: {e}")
        sys.exit(1)
    except FileNotFoundError:
        print(f"Error: {script} not found")
        sys.exit(1)

if __name__ == "__main__":
    main()
