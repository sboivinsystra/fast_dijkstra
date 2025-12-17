#!/bin/bash

REPO="https://github.com/sboivinsystra/fast_dijkstra"
TEMP_DIR="pypi_upload_temp"

# --- Load .env file ---
if [ -f .env ]; then
    echo "Loading variables from .env..."
    export $(grep -v '^#' .env | xargs)
else
    echo "ERROR: .env file not found!"
    exit 1
fi

# 2. Get the Tag (User Input)
# Check if tag was passed as an argument (e.g., ./upload.sh v1.0.2)
if [ -n "$1" ]; then
    TAG=$1
else
    # Otherwise, ask the user
    read -p "Enter the GitHub Tag to download the release (e.g., v1.0.1): " TAG
fi

if [ -z "$TAG" ]; then
    echo "❌ ERROR: No tag provided."
    exit 1
fi


# temp directory
echo "Creating temporary directory..."
rm -rf $TEMP_DIR
mkdir -p $TEMP_DIR/dist
cd $TEMP_DIR


# 4. Download Assets from GitHub via API
echo "Fetching asset list for release $TAG..."
# This API call gets the download URLs for all files in that release
API_URL="https://api.github.com/repos/sboivinsystra/fast_dijkstra/releases/tags/$TAG"
ASSET_URLS=$(curl -s $API_URL | grep "browser_download_url" | cut -d '"' -f 4)

# Safety Check: Exit if no assets found
if [ -z "$ASSET_URLS" ]; then
    echo "ERROR: No assets found for tag $TAG. Check your username/repo/tag."
    exit 1
fi

echo "Downloading assets..."
for url in $ASSET_URLS; do
    echo "Getting: $(basename $url)"
    curl -L -O --output-dir dist "$url"
done



# Check if any files actually exist in dist/
COUNT=$(ls -1 dist/*.whl 2>/dev/null | wc -l)
if [ "$COUNT" -eq 0 ]; then
    echo "❌ ERROR: No .whl files found in the download directory."
    exit 1
fi

# Create a temporary Virtual Environment
echo "install twine in Venv"
python3 -m venv venv
source venv/bin/activate
pip install twine 

#  Metadata Check
echo "Checking metadata with twine..."
python3 -m twine check dist/*

#  Publish
echo "Ready to publish to PyPI."
read -p "Continue with upload? (y/n) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    python3 -m twine upload dist/*
fi

cd ..
rm -rf $TEMP_DIR # Uncomment to auto-delete after finish