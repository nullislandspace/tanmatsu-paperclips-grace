# Paperclips Save Game Format

Binary tagged format for Universal Paperclips (Tanmatsu Edition) save files.
Inspired by Minecraft's Named Binary Tag (NBT) format, but simplified for
embedded use. No compression, no lists-of-compounds — just named scalar values
and named groups.

## File Header

| Offset | Size | Type     | Value        | Description                        |
|--------|------|----------|--------------|------------------------------------|
| 0      | 4    | char[4]  | `"CLIP"`     | Magic bytes (0x43 0x4C 0x49 0x50)  |
| 4      | 4    | uint32   | `0x01020304` | Endian sentinel (little-endian)     |
| 8      | 2    | uint16   | 4            | Format version (current: 4)         |

The endian sentinel is stored as the four bytes `04 03 02 01` in the file
(little-endian). A reader should check this value: if it reads back as
`0x01020304`, the file matches the reader's byte order. If it reads as
`0x04030201`, the byte order is swapped and all multi-byte values must be
byte-swapped on read.

After the 10-byte header, the root compound tag follows immediately.

## Tag Types

| ID   | Name     | Payload                               |
|------|----------|---------------------------------------|
| 0x00 | END      | (none) — terminates a compound        |
| 0x01 | INT32    | 4 bytes, signed, little-endian        |
| 0x02 | DOUBLE   | 8 bytes, IEEE 754, little-endian      |
| 0x03 | STRING   | uint16 length (LE) + UTF-8 bytes      |
| 0x04 | COMPOUND | sequence of named tags, closed by END |
| 0x05 | INT64    | 8 bytes, signed, little-endian        |

## Tag Encoding

Every tag (except END) is encoded as:

```
[type: 1 byte] [name_length: 2 bytes LE] [name: N bytes] [payload]
```

The END tag is a single `0x00` byte with no name or payload.

Strings in tag names and STRING payloads are UTF-8 encoded, not
null-terminated. The length prefix gives the byte count.

## Compounds

A COMPOUND tag contains zero or more child tags, terminated by an END tag.
Compounds can be nested. The tag names within a compound should be unique,
but readers should handle duplicates gracefully (last value wins).

## File Structure

The root of the file (after the header) is a single COMPOUND tag named
`"root"`. It contains all save data as named children.

### Peek Header

The first child of root is a `"peek"` compound containing fields needed
for quick slot preview (save/load menu). These fields are **not duplicated**
elsewhere in the file — the full load reads them from the peek compound.

```
COMPOUND "root"
  COMPOUND "peek"
    DOUBLE  "clips"              = 863489656.375
    DOUBLE  "ticks"              = 250000.0
    INT64   "lastSaveTimestamp"  = 1743379200
    INT32   "humanFlag"          = 1
    INT32   "spaceFlag"          = 0
    INT32   "dismantle"          = 0
  END
```

### Top-Level Fields

All other scalar game state values are stored as direct children of root
with their C field name as the tag name. Examples:

```
  DOUBLE  "wire"           = 85115.625
  INT32   "processors"     = 22
  DOUBLE  "trust"          = 104.0
  ...
```

### PRNG State

```
  COMPOUND "prng"
    INT64 "s0" = ...
    INT64 "s1" = ...
    INT64 "s2" = ...
    INT64 "s3" = ...
  END
```

### Quantum Chips

Each chip is a named compound `"qChip0"` through `"qChip9"`:

```
  COMPOUND "qChip0"
    DOUBLE "waveSeed" = ...
    DOUBLE "value"    = ...
    INT32  "active"   = ...
  END
```

### Stocks

Each stock is a named compound `"stock0"` through `"stock4"`:

```
  COMPOUND "stock0"
    INT32   "id"     = ...
    STRING  "symbol" = "ABCD"
    DOUBLE  "price"  = ...
    DOUBLE  "amount" = ...
    DOUBLE  "total"  = ...
    DOUBLE  "profit" = ...
    INT32   "age"    = ...
  END
```

### Battles

Each battle is a named compound `"battle0"`:

```
  COMPOUND "battle0"
    INT32   "id"            = ...
    DOUBLE  "clipProbes"    = ...
    DOUBLE  "drifterProbes" = ...
    INT32   "victory"       = ...
    INT32   "loss"          = ...
    INT32   "whiteFlag"     = ...
    DOUBLE  "territory"     = ...
    INT32   "reportCount"   = ...
    INT32   "garbageFlag"   = ...
  END
```

### Income Tracker

```
  COMPOUND "incomeTracker"
    DOUBLE "0" = ...
    DOUBLE "1" = ...
    ...
    DOUBLE "9" = ...
  END
```

### Battle Numbers

```
  COMPOUND "battleNumbers"
    INT32 "0" = 1
    INT32 "1" = 1
    ...
  END
```

### Strategy Active

```
  COMPOUND "stratActive"
    INT32 "0" = 1
    INT32 "1" = 0
    ...
  END
```

### Projects

Projects are stored by name, not by slot index. Each project that has been
interacted with (flag set or uses changed from default) is stored:

```
  COMPOUND "projects"
    COMPOUND "PROJ_RAPID_KEYPRESSING"
      INT32 "flag" = 1
      INT32 "uses" = 0
    END
    COMPOUND "PROJ_OFFLINE_1"
      INT32 "flag" = 1
      INT32 "uses" = 0
    END
    ...
  END
```

Projects not present in the save file are assumed to have `flag=0, uses=1`
(the defaults from `game_state_init()`).

When loading, unknown project names are silently ignored (allows downgrading).
When saving, project names are resolved from the `PROJ_*` defines in
`game_projects.h`.

### Console Messages

```
  COMPOUND "messages"
    STRING "0" = "AutoClippers available for purchase"
    STRING "1" = "500 clips created in 2 minutes 15 seconds"
    ...
  END
  INT32 "messageCount" = 2
```

## Versioning and Compatibility

- **Adding fields**: New fields get defaults from `game_state_init()` when
  loading older saves that lack them. No migration needed.
- **Removing fields**: Unknown tags are silently skipped on load. Old saves
  with removed fields load cleanly.
- **Renaming fields**: Requires a version check and explicit migration, or
  support both old and new names.
- **Projects**: Stored by name, so slot number changes don't affect saves.

The format version in the header should be bumped when the *meaning* of
existing fields changes (not when fields are added or removed).

## Size Estimate

Each tag has ~3 bytes overhead (type + name length) plus the name string.
With ~200 scalar fields averaging 10-character names, overhead is roughly
200 * 13 = 2.6 KB on top of the raw data (~3.5 KB). Total save size is
approximately 6-7 KB — well within SD card / flash constraints.
