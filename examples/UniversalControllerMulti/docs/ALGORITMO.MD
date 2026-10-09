# UniversalControllerMulti — Game Algorithm

## Overview

Multiple UniversalController puzzles running on a single board. Each puzzle has its own JSON configuration file. The LogicManager manages the orchestration between them.

## Configuration

- **puzzles[]**: array of puzzle configurations
- **gameMode**: PARALLEL (all simultaneous) or SEQUENTIAL (one after another)

## States

IDLE → RUNNING → SOLVED (all complete)

## Flow

1. PARALLEL: all puzzles start together, solves when all complete
2. SEQUENTIAL: puzzle N+1 only starts after puzzle N solves
3. Each individual puzzle follows the UniversalController logic

## AP Mode + WiFi

Network configuration via web portal at 192.168.4.1.
