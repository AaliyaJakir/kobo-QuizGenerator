#!/bin/sh

# Path to binaries on Kobo
CURL_BIN="/mnt/onboard/.niluje/usbnet/bin/curl"
JQ_BIN="/mnt/onboard/.niluje/usbnet/bin/jq"
CONFIG_FILE="/mnt/onboard/.adds/pkm/config.txt"
OUTPUT_DIR="/mnt/onboard/.adds/quiz"
OUTPUT_FILE="$OUTPUT_DIR/books.json"

# Check if config file exists and source it
if [ -f "$CONFIG_FILE" ]; then
    . "$CONFIG_FILE"
else
    echo "Error: Config file not found at $CONFIG_FILE" >&2
    exit 1
fi

# Check if jq exists
if [ ! -f "$JQ_BIN" ]; then
    echo "Error: jq not found at $JQ_BIN" >&2
    exit 1
fi

# Create output directory if it doesn't exist
mkdir -p "$OUTPUT_DIR"

# Fetch book list
response=$($CURL_BIN -s "$SERVER_URL/books")

# Check if response is valid JSON
if echo "$response" | $JQ_BIN . >/dev/null 2>&1; then
    # Save response to file
    echo "$response" > "$OUTPUT_FILE"
    echo "Successfully updated books list"
else
    echo "Error: Invalid response from server" >&2
    exit 1
fi 