>  **After Roblox Updates**
> Roblox/Sober updates may reset or break FFlags. You’ll need to reapply them using `fflag`.

---

# Sober FFlag Editor (CLI)

## Installation

1. Compile the source:

   ```bash
   gcc $(pkg-config --cflags --libs json-c) -o fflag SoberFFlagEditor.c
   ```
2. Move it into your `$PATH`:

   ```bash
   sudo cp fflag /usr/local/bin/
   sudo chmod +x /usr/local/bin/fflag
   ```

Now you can use `fflag` like any other system command.

---

## Usage

```bash
fflag add <FlagName> <Value>
fflag replace <FlagName> <Value>
fflag remove <FlagName>
fflag list
```

### Example:

```bash
fflag add DFIntMyCustomFlag 123
fflag replace DFIntMyCustomFlag 456
fflag remove DFIntMyCustomFlag
fflag list
```

---

Note: Precompiled versions of fflag are not guaranteed to work
