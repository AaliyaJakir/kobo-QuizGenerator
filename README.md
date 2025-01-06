# Kobo-QuizGenerator

This Kobo plugin uses AI to generate multiple choice questions from your books. Select any book from your library to get a 3-question quiz, complete with scoring and review functionality.

---

## Installation

1. **Configure Books List**
   Create a JSON file at `/mnt/onboard/.adds/quiz/books.json` with your book list
   
   Sample:
   ```json
   {
       "books": [
           "Franny and Zooey",
           "Atomic Habits",
           "Sapiens"
       ]
   }
   ```

2. **Add Required Files**
   - Put `generateQuiz.sh`, `updateBooks.sh` and `prompts.txt` in `/mnt/onboard/.adds/quiz/`

3. **Set API Configuration**
   Add your API credentials to `/mnt/onboard/.adds/pkm/config.txt`:
   - OPENAI_API_URL
   - OPENAI_API_KEY
   (Note: Currently configured for Azure OpenAI)

4. **Update Kobo**
   Place `KoboRoot.tgz` in your Kobo's `.kobo` folder to update your device.

### Updating your books

To update your books, you'll need to:
- Get `calibre_kobo_server.py` which is available at the kobo-syllabusFetch repository -- This has an endpoint to update books.json with your books (unfortunately koreader.sqlite doesn't easily offer this information.. )

---

## Development

To make changes and rebuild the plugin:
```bash
docker run -u $(id -u):$(id -g) --volume="$PWD:$PWD" --entrypoint=make --workdir="$PWD" --env=HOME --rm -it ghcr.io/pgaskin/nickeltc:1 NAME=SyllabusFetch
