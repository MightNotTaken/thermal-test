#!/bin/bash

# JSON Validation Test Script

echo "=== JSON Configuration Test ==="
echo ""

# Check if config file exists
if [ ! -f "config/jetson_thermal.conf" ]; then
    echo "Error: config/jetson_thermal.conf not found"
    exit 1
fi

echo "1. Checking JSON syntax with python..."
python3 -c "
import json
import sys
try:
    with open('config/jetson_thermal.conf', 'r') as f:
        data = json.load(f)
    print('✅ JSON is valid')
    print('Keys found:', list(data.keys()))
except json.JSONDecodeError as e:
    print('❌ JSON syntax error:', e)
    sys.exit(1)
except Exception as e:
    print('❌ Error:', e)
    sys.exit(1)
"

echo ""
echo "2. Checking JSON syntax with jq..."
if command -v jq >/dev/null 2>&1; then
    if jq empty config/jetson_thermal.conf 2>/dev/null; then
        echo "✅ JSON is valid (jq validation)"
    else
        echo "❌ JSON is invalid (jq validation)"
        echo "jq error output:"
        jq empty config/jetson_thermal.conf
    fi
else
    echo "⚠️  jq not available, skipping jq validation"
fi

echo ""
echo "3. Raw file content (first 10 lines):"
head -10 config/jetson_thermal.conf

echo ""
echo "4. File size and permissions:"
ls -la config/jetson_thermal.conf

echo ""
echo "=== JSON Test Complete ==="
