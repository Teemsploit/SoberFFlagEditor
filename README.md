#  Sober FFlag Editor (CLI)  
*A simple command-line tool to manage Roblox FastFlags (FFlags) for the [Sober Linux client](https://github.com/vinegarhq/sober).*

---

##  Features

-  Add, replace, or remove FFlags with ease  
-  List current FFlags applied to Sober  
-  Fully compatible with Linux systems  
-  Lightweight and depends only on `json-c`  

---

## Installation

1. **Compile the source code**  
   Make sure you have `json-c` installed (`libjson-c-dev` on Debian/Ubuntu):

   ```bash
   gcc $(pkg-config --cflags --libs json-c) -o fflag SoberFFlagEditor.c
   ```

2. **Install to your PATH**

   ```bash
   sudo cp fflag /usr/local/bin/
   sudo chmod +x /usr/local/bin/fflag
   ```

## Or

```bash
git clone https://github.com/Teemsploit/SoberFFlagEditor.git
cd SoberFFlagEditor
./install.sh
```

You can now run `fflag` like any other command.

---

## Usage

```bash
fflag add <FlagName> <Value>       # Adds a new FFlag
fflag replace <FlagName> <Value>   # Replaces value of an existing FFlag
fflag remove <FlagName>            # Removes an FFlag
fflag list                         # Lists all current FFlags
```

### Example

```bash
fflag add DFIntMyCustomFlag 123
fflag replace DFIntMyCustomFlag 456
fflag remove DFIntMyCustomFlag
fflag list
```

---

## Notes

- **After Roblox/Sober Updates**:  
  FFlags may be reset or broken after updates. Simply reapply them using `fflag`.

- **Precompiled Versions**:  
  Precompiled Versions are not guaranteed to work.

---

## What are fflags

Roblox Fast Flags are a type of configuration setting used internally by Roblox engineers to quickly enable or disable features and functionalities within the Roblox platform. These flags allow the developers to test new features, make updates, and address issues without needing to deploy a full update to the platform.
