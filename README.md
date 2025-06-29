# Discworld Noir Fix
Some patches to fix the Discworld Noir game. Should run in windowed mode on modern systems.

# Install
Copy the `ddraw.dll` file to the game directory.

# Notes
- This only works on the non-DRM version of the game. If you have an `.icd` file in your game directory it won't work
    - There does exist a DRM-free version of the game _somewhere_ on the internet, but I won't link it here. You need the version that has a text file called `Discworld Noir_Last Update.txt` on the first CD
- The game should set itself to windowed mode, if not ensure the following registry key is set to `Yes` - `HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\Perfect Entertainment\Discworld Noir\Start Windowed`
- You may still need to have an ISO for the first CD mounted (todo)
