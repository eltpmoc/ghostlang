import os
cmd = 'curl.exe -q --silent --fail --proto =https --connect-timeout 4 --max-time 10 --max-filesize 2097152 --user-agent "GhostLangLocalAI/3.3" --write-out "\\nGHOST_HTTP_CODE:%{http_code}" "https://pt.wikipedia.org/w/api.php?action=query&list=search&srlimit=1&format=xml&srsearch=napoleao%20bonaparte"'
print(f"Running: {cmd}")
with os.popen(cmd) as f:
    out = f.read()
    print("Output:")
    print(out)
