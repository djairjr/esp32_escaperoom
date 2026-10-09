# KeypadCodeMultiple — Game Algorithm

## Overview

Multi-code version of KeypadCodeEntry. Accepts multiple codes configured via JSON. Any valid code solves the puzzle. Useful for multiple players or multiple solutions.

## Configuration (JSON: keypadmulti.json)

- **codes**: array of up to 8 four-digit codes
- **codeCount**: how many codes are active
- **Hardware**: lockPin, neoCount, volume

## States

IDLE → ENTERING → SOLVED/ERROR → IDLE

## Flow

Same as KeypadCodeEntry, but it checks against all codes in the array. Any match solves it.

## AP Mode + WiFi

Network configuration via web portal at 192.168.4.1.
