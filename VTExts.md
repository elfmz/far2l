# Far2l Terminal Extensions Protocol Specification

## 1. Introduction

The `far2l` terminal extensions protocol provides a mechanism for bidirectional communication between a `far2l` internal virtual terminal (VT) and a terminal client application. It enables advanced features not available through standard terminal escape codes, such as native clipboard integration, rich desktop notifications, and direct graphics rendering.

Communication is achieved through specially formatted ANSI escape sequences, where the payload is a Base64-encoded binary stack.

## 2. Core Concepts

### 2.1. Command and Notification Structure

The protocol defines two primary message types, distinguished by their prefix:

-   **Client-to-Server (Commands):** Sent from the terminal application to `far2l`.
    > `\x1B_far2l:<payload>\x07`
-   **Server-to-Client (Notifications):** Sent from `far2l` to the terminal application.
    > `\x1B_f2l:<payload>\x07`

Where:
- `\x1B` is the `ESC` character.
- `<payload>` is the Base64-encoded binary stack.
- `\x07` is the `BEL` character, which terminates the sequence.

### 2.2. The Stack Serializer

The payload of every message is a binary **stack**. This follows a **Last-In, First-Out (LIFO)** data structure.

This is the most critical concept to understand: **arguments must be pushed onto the stack in the reverse order of how they are to be read.**

For example, if the server needs to read `Argument A`, then `Argument B`, the client must construct the stack by first pushing `Argument B`, then `Argument A`.

> **Note:** Throughout this document, arguments for "In" (client to server) and "Out" (server to client) stacks are listed in the order they are **popped off the stack (top to bottom)**. This represents the logical order of processing, not the order of construction.

### 2.3. Data Types

-   **Integers:** All integer types (`uint8_t`, `uint16_t`, `uint32_t`, `uint64_t`, etc.) are encoded in **little-endian** byte order.
-   **Strings:** A string is pushed onto the stack as the raw byte sequence followed by its size as a `uint32_t`.
-   **Raw Data:** A raw byte buffer is pushed onto the stack without a size prefix. Its size is typically inferred from other arguments (e.g., image dimensions).

### 2.4. Request/Response Mechanism (Client-to-Server)

Client-to-Server commands use an 8-bit Request ID to manage responses. This ID is **always the last item pushed onto the stack**.

-   **Request ID = 0:** An asynchronous command. The server will not send a reply.
-   **Request ID > 0:** A synchronous command. After processing, the server will send a reply using the same command structure (`\x1B_far2l:...`). The reply stack will contain the same Request ID on top, followed by any return values.

## 3. Protocol Handshake

Before sending any commands, the client must enable the extension protocol.

1.  **Client Sends Activation:** The client sends the raw sequence:
    ```
    \x1B_far2l1\x07
    ```
2.  **Server Acknowledges:** The server (`far2l`) responds with:
    ```
    \x1B_far2lok\x07
    ```
The client must wait for this acknowledgment before proceeding.

NB! Better approach is to add `\x1b[5n` after `\x1B_far2l1\x07` that will cause non-far2l terminal to respond with (typically) `\x1b[0n`. This avoids the use of timeouts, the counting of which can be long in terminals that do not support far2l extensions.

## 4. Client-to-Server Commands (`FARTTY_INTERACT_*`)

These commands are sent by the client to request an action from `far2l`.

---

### `FARTTY_INTERACT_CHOOSE_EXTRA_FEATURES` ('x')

Declares that the client supports a set of optional features. `far2l` may change its behavior accordingly. This should be sent after the initial handshake.

-   **In Stack:**
    | Argument        | Type       | Description                                  |
    | --------------- | ---------- | -------------------------------------------- |
    | Feature Flags   | `uint64_t` | A bitmask of `FARTTY_FEAT_*` flags.          |
-   **Out Stack:** None.

`FARTTY_FEAT_*` (Client Features)

-   `FARTTY_FEAT_COMPACT_INPUT` (0x01): Client supports receiving compact input events.
-   `FARTTY_FEAT_TERMINAL_SIZE` (0x02): Client supports receiving terminal size events via this protocol (in addition to `SIGWINCH`).

---

### `FARTTY_INTERACT_CONSOLE_ADHOC_QEDIT` ('e')

Initiates a quick-edit (text selection) operation in `far2l` starting from the last mouse click position.

-   **In Stack:** None.
-   **Out Stack:** None.

---

### `FARTTY_INTERACT_WINDOW_MAXIMIZE` ('M')

Requests that the `far2l` window be maximized.

-   **In Stack:** None.
-   **Out Stack:** None.

---

### `FARTTY_INTERACT_WINDOW_RESTORE` ('m')

Requests that the `far2l` window be restored from a maximized state.

-   **In Stack:** None.
-   **Out Stack:** None.

---

### `FARTTY_INTERACT_SET_CURSOR_HEIGHT` ('h')

Changes the height of the terminal cursor.

-   **In Stack:**
    | Argument       | Type      | Description                         |
    | -------------- | --------- | ----------------------------------- |
    | Cursor Height  | `uint8_t` | Height in percent (0-100).          |
-   **Out Stack:** None.

---

### `FARTTY_INTERACT_GET_WINDOW_MAXSIZE` ('w')

Gets the maximum possible window size in character cells (columns and rows).

-   **In Stack:** None.
-   **Out Stack:**
    | Argument    | Type       | Description                 |
    | ----------- | ---------- | --------------------------- |
    | Height      | `uint16_t` | Maximum height in rows.     |
    | Width       | `uint16_t` | Maximum width in columns.   |

---

### `FARTTY_INTERACT_DESKTOP_NOTIFICATION` ('n')

Displays a desktop notification.

-   **In Stack:**
    | Argument  | Type     | Description             |
    | --------- | -------- | ----------------------- |
    | Title     | `string` | The notification title. |
    | Text      | `string` | The notification body.  |
-   **Out Stack:** None.

---

### `FARTTY_INTERACT_SET_FKEY_TITLES` ('f')

Sets the titles for the F-key bar (e.g., on a Mac Touch Bar). The stack contains 12 elements, from F12 down to F1.

-   **In Stack:**
    | Argument           | Type          | Description                                                                     |
    | ------------------ | ------------- | ------------------------------------------------------------------------------- |
    | 12 x Title Blocks | (see below)  | A sequence of 12 blocks, one for each F-key from F12 down to F1. Each block contains a state and an optional title. If state is `0`, the title is cleared. If state is non-zero, the following string is the new title. |
-   **Out Stack:**
    | Argument  | Type   | Description                                 |
    | --------- | ------ | ------------------------------------------- |
    | Success   | `bool` | `true` if the operation was supported and succeeded. |

---

### `FARTTY_INTERACT_GET_COLOR_PALETTE` ('p')

Queries the color capabilities of the host terminal.

-   **In Stack:** None.
-   **Out Stack:**
    | Argument      | Type      | Description                                                |
    | ------------- | --------- | ---------------------------------------------------------- |
    | Color Bits    | `uint8_t` | Maximum supported color resolution (e.g., 4, 8, or 24).    |
    | Reserved      | `uint8_t` | Reserved for future use (currently zero).                   |

---

### `FARTTY_INTERACT_CLIPBOARD` ('c')

Namespace for all clipboard-related operations.

-   **In Stack:**
    | Argument     | Type   | Description                                         |
    | ------------ | ------ | --------------------------------------------------- |
    | Sub-command  | `char` | A character identifying the specific clipboard action. |
    | ...          | ...    | Arguments specific to the sub-command.              |
-   **Out Stack:** Varies by sub-command.

#### Clipboard Operations

##### `FARTTY_INTERACT_CLIP_OPEN` ('o')
Authorizes and opens the clipboard for subsequent operations.

-   **In Stack:**
    | Argument     | Type     | Description                                         |
    | ------------ | -------- | --------------------------------------------------- |
    | Passcode     | `string` | A unique client identifier (32-256 chars).         |
-   **Out Stack:**
    | Argument          | Type       | Description                                                                      |
    | ----------------- | ---------- | -------------------------------------------------------------------------------- |
    | Status            | `int8_t`   | `1` for success, `0` for failure, `-1` for access denied.                          |
    | Server Features   | `uint64_t` | (Optional) A bitmask of `FARTTY_FEATCLIP_*` flags supported by the server.         |

`FARTTY_FEATCLIP_*` (Server Clipboard Features)

-   `FARTTY_FEATCLIP_DATA_ID` (0x01): Server can provide a unique ID for clipboard data, allowing for client-side caching.
-   `FARTTY_FEATCLIP_CHUNKED_SET` (0x02): Server supports setting clipboard data in multiple chunks.

##### `FARTTY_INTERACT_CLIP_CLOSE` ('c')
Closes the clipboard, finalizing the transaction.

-   **In Stack:** None.
-   **Out Stack:**
    | Argument   | Type     | Description                                         |
    | ---------- | -------- | --------------------------------------------------- |
    | Status     | `int8_t` | `1` for success, `0` for failure, `-1` if not open.   |

##### `FARTTY_INTERACT_CLIP_EMPTY` ('e')
Clears the clipboard content.

-   **In Stack:** None.
-   **Out Stack:**
    | Argument   | Type     | Description                                         |
    | ---------- | -------- | --------------------------------------------------- |
    | Status     | `int8_t` | `1` for success, `0` for failure, `-1` if not open.   |

##### `FARTTY_INTERACT_CLIP_ISAVAIL` ('a')
Checks if a specific data format is available on the clipboard.

-   **In Stack:**
    | Argument   | Type       | Description                                         |
    | ---------- | ---------- | --------------------------------------------------- |
    | Format ID  | `uint32_t` | The format to check (e.g., `1` for CF_TEXT).        |
-   **Out Stack:**
    | Argument   | Type     | Description                                         |
    | ---------- | -------- | --------------------------------------------------- |
    | Available  | `int8_t` | `1` if available, `0` if not.                         |

##### `FARTTY_INTERACT_CLIP_SETDATA` ('s')
Puts data onto the clipboard.

-   **In Stack:**
    | Argument   | Type        | Description                                                               |
    | ---------- | ----------- | ------------------------------------------------------------------------- |
    | Format ID  | `uint32_t`  | The ID of the data format.                                                |
    | Data Size  | `uint32_t`  | The size of the data buffer that follows.                                 |
    | Data       | `raw bytes` | The data buffer. (Prepended by any data from previous `..._SETDATACHUNK` calls). |
-   **Out Stack:**
    | Argument   | Type       | Description                                  |
    | ---------- | ---------- | -------------------------------------------- |
    | Status     | `int8_t`   | `1` for success, `0` for failure, `-1` if not open. |
    | Data ID    | `uint64_t` | (Optional) A unique ID for the data (e.g., CRC64). |

##### `FARTTY_INTERACT_CLIP_SETDATACHUNK` ('S')
(Optional) Sends a chunk of data to the server. This allows for sending large amounts of data in the background, which can be canceled. This command can be used multiple times before a final `FARTTY_INTERACT_CLIP_SETDATA` call appends the last part of the data and finalizes the operation.

> **Note:** This feature is optional and can only be used if the server reported the `FARTTY_FEATCLIP_CHUNKED_SET` flag during the `CLIP_OPEN` response.

-   **In Stack:**
    | Argument           | Type       | Description                                                                                                        |
    | ------------------ | ---------- | ------------------------------------------------------------------------------------------------------------------ |
    | Encoded Chunk Size | `uint16_t` | The size of the chunk's data, right-shifted by 8 bits. The actual size is `value << 8`. A value of `0` cancels all pending chunks. |
    | Chunk Data         | `raw bytes`| The raw byte data for this chunk.                                                                                  |
-   **Out Stack:** None.

##### `FARTTY_INTERACT_CLIP_GETDATA` ('g')
Retrieves data from the clipboard.

-   **In Stack:**
    | Argument   | Type       | Description                  |
    | ---------- | ---------- | ---------------------------- |
    | Format ID  | `uint32_t` | The format to retrieve.      |
-   **Out Stack:**
    | Argument   | Type        | Description                                  |
    | ---------- | ----------- | -------------------------------------------- |
    | Data Size  | `uint32_t`  | The size of the retrieved data. `0` on failure, `-1` if not open. |
    | Data       | `raw bytes` | The data buffer.                             |
    | Data ID    | `uint64_t`  | (Optional) A unique ID for the data.         |

---

### `FARTTY_INTERACT_IMAGE` ('i')

Namespace for all image rendering operations.

-   **In Stack:**
    | Argument     | Type   | Description                                         |
    | ------------ | ------ | --------------------------------------------------- |
    | Sub-command  | `char` | A character identifying the specific image action.    |
    | ...          | ...    | Arguments specific to the sub-command.              |
-   **Out Stack:** Varies by sub-command.

#### Image Operations

##### `FARTTY_INTERACT_IMAGE_CAPS` ('c')
Queries the terminal's image rendering capabilities.

-   **In Stack:** None.
-   **Out Stack:**
    | Argument          | Type       | Description                                  |
    | ----------------- | ---------- | -------------------------------------------- |
    | Capabilities      | `uint64_t` | A bitmask of `WP_IMGCAP_*` flags, see below. |
    | Cell Width (px)   | `uint16_t` | The width of a character cell in pixels.     |
    | Cell Height (px)  | `uint16_t` | The height of a character cell in pixels.    |

`WP_IMGCAP_*` (Rendering Capabilities)

-   `WP_IMGCAP_RGBA` (0x01): Client supports supports WP_IMG_RGB/WP_IMG_RGBA formats (see below).
-   `WP_IMGCAP_SCROLL` (0x02): Client supports existing image scrolling.
-   `WP_IMGCAP_ROTATE` (0x03): Client supports existing image rotation.

##### `FARTTY_INTERACT_IMAGE_SET` ('s')
Uploads and displays an image.

-   **In Stack:**
    | Argument      | Type        | Description                                  |
    | ------------- | ----------- | -------------------------------------------- |
    | Image ID      | `string`    | A unique identifier for the image.           |
    | Flags         | `uint64_t`  | The image format flags, see below.           |
    | Position X    | `uint16_t`  | The horizontal character column.             |
    | Position Y    | `uint16_t`  | The vertical character row.                  |
    | Image Width   | `uint32_t`  | Image width in pixels.                       |
    | Image Height  | `uint32_t`  | Image height in pixels.                      |
    | Image Data    | `raw bytes` | The raw pixel data.                          |
-   **Out Stack:**
    | Argument   | Type      | Description                                  |
    | ---------- | --------- | -------------------------------------------- |
    | Success    | `uint8_t` | `1` on success, `0` on failure.                |

`WP_IMG_*` (Image Format Flags)

-   `WP_IMG_RGBA` (0x00): Supported if WP_IMGCAP_RGBA was set
-   `WP_IMG_RGB` (0x01): Supported if WP_IMGCAP_RGBA was set

Flags below are supported only if WP_IMGCAP_SCROLL was reported. They are intended to scroll existing image instead of displaying a new one.

-   `WP_IMG_SCROLL_AT_LEFT` (0x10000): Left->right scrolling, sending rectangle to insert at left
-   `WP_IMG_SCROLL_AT_RIGHT` (0x20000): Right->left scrolling, sending rectangle to insert at right
-   `WP_IMG_SCROLL_AT_TOP` (0x30000): Top->bottom scrolling, sending rectangle to insert at top
-   `WP_IMG_SCROLL_AT_BOTTOM` (0x40000): Bottom->top scrolling, sending rectangle to insert at bottom

##### `FARTTY_INTERACT_IMAGE_DEL` ('d')
Removes a previously displayed image.

-   **In Stack:**
    | Argument   | Type     | Description                           |
    | ---------- | -------- | ------------------------------------- |
    | Image ID   | `string` | The identifier of the image to remove. |
-   **Out Stack:**
    | Argument   | Type      | Description                                  |
    | ---------- | --------- | -------------------------------------------- |
    | Success    | `uint8_t` | `1` on success, `0` on failure.                |

##### `FARTTY_INTERACT_IMAGE_ROT` ('r')
Rotates and repositions a previously displayed image.

-   **In Stack:**
    | Argument      | Type       | Description                                                                 |
    | ------------- | ---------- | --------------------------------------------------------------------------- |
    | Image ID      | `string`   | The unique identifier of the image to rotate.                               |
    | Position X    | `uint16_t` | The new horizontal character column for the image's top-left corner.        |
    | Position Y    | `uint16_t` | The new vertical character row for the image's top-left corner.             |
    | Rotation Angle| `uint8_t`  | The angle of rotation in 90-degree increments (`0`=0°, `1`=90°, `2`=180°, `3`=270°). |
-   **Out Stack:**
    | Argument   | Type      | Description                                  |
    | ---------- | --------- | -------------------------------------------- |
    | Success    | `uint8_t` | `1` on success, `0` on failure.                |

## 5. Server-to-Client Notifications (`FARTTY_INPUT_*`)

These messages are sent asynchronously from `far2l` to the client to report events like keyboard and mouse input. They do not have a Request ID and do not expect a reply.

---

### `FARTTY_INPUT_MOUSE` ('M') / `FARTTY_INPUT_MOUSE_COMPACT` ('m')

Reports a mouse event. The compact version uses smaller data types if negotiated via `FARTTY_FEAT_COMPACT_INPUT`.

-   **Payload Stack (Normal):**
    | Argument         | Type       | Description (`MOUSE_EVENT_RECORD`) |
    | ---------------- | ---------- | -------------------------------- |
    | Event Flags      | `uint32_t` | `dwEventFlags`                   |
    | Control Key State| `uint32_t` | `dwControlKeyState`              |
    | Button State     | `uint32_t` | `dwButtonState`                  |
    | Position Y       | `int16_t`  | `dwMousePosition.Y`              |
    | Position X       | `int16_t`  | `dwMousePosition.X`              |

-   **Payload Stack (Compact):**
    | Argument         | Type       | Description (`MOUSE_EVENT_RECORD`) |
    | ---------------- | ---------- | -------------------------------- |
    | Event Flags      | `uint8_t`  | `dwEventFlags`                   |
    | Control Key State| `uint8_t`  | `dwControlKeyState`              |
    | Button State     | `uint16_t` | `dwButtonState` (encoded)        |
    | Position Y       | `int16_t`  | `dwMousePosition.Y`              |
    | Position X       | `int16_t`  | `dwMousePosition.X`              |

---

### `FARTTY_INPUT_KEYDOWN` ('K') / `FARTTY_INPUT_KEYUP` ('k')

Reports a key press or release event.

-   **Payload Stack (Normal):**
    | Argument         | Type       | Description (`KEY_EVENT_RECORD`)   |
    | ---------------- | ---------- | -------------------------------- |
    | Unicode Char     | `uint32_t` | `uChar.UnicodeChar` (UTF-32)     |
    | Control Key State| `uint32_t` | `dwControlKeyState`              |
    | Virtual Scan Code| `uint16_t` | `wVirtualScanCode`               |
    | Virtual Key Code | `uint16_t` | `wVirtualKeyCode`                |
    | Repeat Count     | `uint16_t` | `wRepeatCount`                   |

-   **Payload Stack (Compact):**
    | Argument         | Type       | Description (`KEY_EVENT_RECORD`)   |
    | ---------------- | ---------- | -------------------------------- |
    | Unicode Char     | `uint16_t` | `uChar.UnicodeChar` (fit to UTF-16) |
    | Control Key State| `uint16_t` | `dwControlKeyState` (fit to 16 bits) |
    | Virtual Key Code | `uint8_t`  | `wVirtualKeyCode` (fit to 8 bits)   |

---

### `FARTTY_INPUT_TERMINAL_SIZE` ('S')

Reports a change in the terminal dimensions.

-   **Payload Stack:**
    | Argument   | Type       | Description            |
    | ---------- | ---------- | ---------------------- |
    | Width      | `uint16_t` | New width in columns.  |
    | Height     | `uint16_t` | New height in rows.    |

