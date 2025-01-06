#!/bin/sh

# Source environment variables
if [ -f "/mnt/onboard/.adds/pkm/config.txt" ]; then
    . "/mnt/onboard/.adds/pkm/config.txt"
else
    echo "Error: config.txt not found"
    exit 1
fi

# Check if required environment variables are set
if [ -z "$OPENAI_API_URL" ] || [ -z "$OPENAI_API_KEY" ]; then
    echo "Error: Required environment variables OPENAI_API_URL or OPENAI_API_KEY are not set"
    exit 1
fi

# Enable error logging
exec 2>/mnt/onboard/.adds/quiz/quiz_error.log

# Debugging: Print to check if the script is running
echo "Running the script..."

# Define paths
CURL_BIN="/mnt/onboard/.niluje/usbnet/bin/curl"
JQ_BIN="/mnt/onboard/.niluje/usbnet/bin/jq"
QUESTIONS_FILE="/mnt/onboard/.adds/quiz/quiz_questions.json"
PROMPTS_FILE="/mnt/onboard/.adds/quiz/prompts.txt"

# Check if dependencies exist
if [ ! -f "$CURL_BIN" ]; then
    echo "Error: curl not found at $CURL_BIN"
    exit 1
fi

if [ ! -f "$JQ_BIN" ]; then
    echo "Error: jq not found at $JQ_BIN"
    exit 1
fi

if [ ! -f "$PROMPTS_FILE" ]; then
    echo "Error: prompts.txt not found at $PROMPTS_FILE"
    exit 1
fi

# Make sure dependencies are executable
chmod +x "$CURL_BIN" "$JQ_BIN"

# Check if we can write to the output file
touch "$QUESTIONS_FILE" 2>/dev/null || {
    echo "Error: Cannot write to $QUESTIONS_FILE"
    exit 1
}

# Get the book title from command line argument
BOOK_TITLE="$1"
if [ -z "$BOOK_TITLE" ]; then
    echo "Error: No book title provided"
    exit 1
fi

# Define the URL and API key
URL="$OPENAI_API_URL"
API_KEY="$OPENAI_API_KEY"

# Create temporary file for JSON data
JSON_FILE="/mnt/onboard/.adds/quiz/quiz_request.json"

# Read prompts from prompts.txt and escape them for JSON
SYSTEM_PROMPT=$(sed -n '/===SYSTEM_PROMPT===/,/===USER_PROMPT===/p' "$PROMPTS_FILE" | grep -v "===.*PROMPT===" | sed 's/"/\\"/g' | tr '\n' ' ')
USER_PROMPT=$(sed -n '/===USER_PROMPT===/,$p' "$PROMPTS_FILE" | grep -v "===.*PROMPT===" | sed 's/"/\\"/g' | tr '\n' ' ')

# Replace book title placeholder
USER_PROMPT=$(echo "$USER_PROMPT" | sed "s/{book_title}/$BOOK_TITLE/g")

# Create JSON data with the prompts using jq to ensure proper JSON formatting
$JQ_BIN -n \
  --arg system "$SYSTEM_PROMPT" \
  --arg user "$USER_PROMPT" \
  '{
    "messages": [
      {"role": "system", "content": $system},
      {"role": "user", "content": $user}
    ],
    "temperature": 0.7
  }' > "$JSON_FILE"

# Send request and save response
echo "Sending request for quiz about: $BOOK_TITLE"
RESPONSE=$($CURL_BIN -s "$URL" \
    -H "Content-Type: application/json" \
    -H "api-key: $API_KEY" \
    -d @"$JSON_FILE")

# Debug: Print raw response
echo "Raw API Response:"
echo "$RESPONSE"

# Check if curl succeeded
if [ $? -ne 0 ]; then
    echo "Error: Failed to make API request"
    exit 1
fi

# Parse response and save questions
echo "$RESPONSE" | $JQ_BIN -r '.choices[0].message.content' > "$QUESTIONS_FILE"

# Check if jq succeeded
if [ $? -ne 0 ]; then
    echo "Error: Failed to parse API response"
    echo "Raw response: $RESPONSE"
    exit 1
fi

# Verify questions file exists and has content
if [ ! -s "$QUESTIONS_FILE" ]; then
    echo "Error: No questions generated"
    echo "Raw response: $RESPONSE"
    exit 1
fi

# Validate JSON format
if ! $JQ_BIN . "$QUESTIONS_FILE" >/dev/null 2>&1; then
    echo "Error: Generated content is not valid JSON"
    exit 1
fi

echo "Questions successfully saved to $QUESTIONS_FILE"

# Clean up temporary file
rm -f "$JSON_FILE"

exit 0
