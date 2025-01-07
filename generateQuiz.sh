#!/bin/sh

# Source environment variables
if [ -f "/mnt/onboard/.adds/pkm/.env" ]; then
    . "/mnt/onboard/.adds/pkm/.env"
else
    echo "Error: .env not found"
    exit 1
fi

# Check if required environment variables are set
if [ -z "$OPENAI_API_URL" ] || [ -z "$OPENAI_API_KEY" ]; then
    echo "Error: Required environment variables OPENAI_API_URL or OPENAI_API_KEY are not set"
    exit 1
fi

# Enable error logging
exec 2>/mnt/onboard/.adds/quiz/quiz_error.log

# Define paths and check dependencies
CURL_BIN="/mnt/onboard/.niluje/usbnet/bin/curl"
JQ_BIN="/mnt/onboard/.niluje/usbnet/bin/jq"
PROMPTS_FILE="/mnt/onboard/.adds/quiz/prompts.txt"

# Check dependencies
if [ ! -f "$CURL_BIN" ] || [ ! -f "$JQ_BIN" ] || [ ! -f "$PROMPTS_FILE" ]; then
    echo "Error: Missing required dependencies"
    exit 1
fi

# Make sure dependencies are executable
chmod +x "$CURL_BIN" "$JQ_BIN"

# Get the book title from command line argument
BOOK_TITLE="$1"
if [ -z "$BOOK_TITLE" ]; then
    echo "Error: No book title provided"
    exit 1
fi

# Define the URL and API key
URL="$OPENAI_API_URL"
API_KEY="$OPENAI_API_KEY"

# Read prompts and escape them for JSON
SYSTEM_PROMPT=$(sed -n '/===SYSTEM_PROMPT===/,/===USER_PROMPT===/p' "$PROMPTS_FILE" | grep -v "===.*PROMPT===" | sed 's/"/\\"/g' | tr '\n' ' ')
USER_PROMPT=$(sed -n '/===USER_PROMPT===/,$p' "$PROMPTS_FILE" | grep -v "===.*PROMPT===" | sed 's/"/\\"/g' | tr '\n' ' ')

# Replace book title placeholder
USER_PROMPT=$(echo "$USER_PROMPT" | sed "s/{book_title}/$BOOK_TITLE/g")

# Create JSON request
REQUEST=$($JQ_BIN -n \
  --arg system "$SYSTEM_PROMPT" \
  --arg user "$USER_PROMPT" \
  '{
    "messages": [
      {"role": "system", "content": $system},
      {"role": "user", "content": $user}
    ],
    "temperature": 0.7
  }')

# Send request and get response
RESPONSE=$($CURL_BIN -s "$URL" \
    -H "Content-Type: application/json" \
    -H "api-key: $API_KEY" \
    -d "$REQUEST")

# Check if curl succeeded
if [ $? -ne 0 ]; then
    echo "Error: Failed to make API request"
    exit 1
fi

# Extract and clean the content, removing markdown code block markers
echo "$RESPONSE" | $JQ_BIN -r '.choices[0].message.content' | sed 's/^```json//g' | sed 's/```$//g'

exit 0
