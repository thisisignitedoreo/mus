
import os

output_file = """
// Created with bundle.py"""

def replace_all(string, allowed, replacement):
    res = ""
    for i in string:
        if i in allowed: res += i
        else: res += replacement
    return res

if __name__ == "__main__":
    if not os.path.isdir("assets"): exit(1)
    files = os.listdir("assets")
    output_file += "\n// Bundled files: " + ", ".join(files) + "\n\n"
    for i in files:
        print(f"Bundling {i}")
        name = replace_all(i.upper(), "ABCDEFGHIJKLMNOPQRSTUVWXYZ", "_")
        f = open(f"assets/{i}", "rb").read()
        output_file += f"const unsigned long _{name}_LENGTH = {len(f)};\n"
        output_file += f"char _{name}[] = " + "{ "
        for b in f:
            output_file += f"0x{hex(b)[2:].upper():0>2}, "
        output_file += "};\n\n"

    open("src/assets.h", "w").write(output_file)
