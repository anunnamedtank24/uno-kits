# bluetooth：介面

| 需要 | 得到 |
|---|---|
| `Link`：UART（`Uno::UnoSoftSerial<RX_PIN, TX_PIN, BAUD, RX_SIZE>`：任兩隻腳，D0 / D1 留給 USB；或 D0 / D1 上 `Uno_serial_buffer.h` 的 `Uno::UnoSerial<BAUD, RX_SIZE, TX_SIZE>`）；`Key`：接 EN / KEY 的輸出腳（`Uno::UnoOutputPin<PIN>`）；`State`：接 STATE 的輸入腳（`Uno::UnoInputPin<PIN>`）；兩隻腳都可以是 `bluetooth::NoPin` | `bluetooth::HC05`：收發，透過 EN 用 AT 命令設名稱、PIN、鮑率、角色；`bluetooth::HC06`：一樣，但沒有 EN / KEY；`bluetooth::Port`：任何模組，本身也是 Link；`bluetooth::Packet`：在位元組流上收發有 CRC-8 的封包 |
| 電腦：Python 3，`pip install pyserial` | `source/packet.py`：透過藍牙序列埠收發同樣的封包 |

給要寫新套件、換主板或換零件的人；用法、接線、注意事項見 [讀我.md](讀我.md)。

## 前提條件

| 項目 | 條件 |
|---|---|
| UART | 模組接在 Link 上：UNO 用 `UnoSoftSerial` 時任兩隻腳（D0 / D1 留給 USB），用 `UnoSerial` 時 D0 / D1（和 USB 共用）；8N1，程式的 `BAUD` = 模組的資料鮑率 |
| 緩衝區 | 建議 `Uno::UnoSerial<BAUD, RX_SIZE, TX_SIZE>`（`Uno_serial_buffer.h`）：中斷收進緩衝區，`loop()` 忙的時候不會漏；沒有緩衝區（輪詢）時硬體只存 2 個位元組，`Packet::update()` / `available()` 要非常常呼叫 |
| static | `Port`、`HC05`、`HC06` 全部是 static：一個模組一個型別；`Packet` 是物件（有緩衝區） |
| AT 命令 | 只在 `setup()` 或很少的時候用：每條會等（HC-05 約 0.1 ～ 1 s，HC-06 約 1 ～ 2.5 s） |
| HC-05 的 EN | 上電時要 LOW：上電時 HIGH 會進 38400 的 AT 模式，`HC05` 的命令和資料都不通 |
| HC-06 | AT 命令只在沒連線時有效；有 STATE 才擋得住連線中送出 |

## 檔案

```
lib/bluetooth/
├── bluetooth.h       入口：全部（程式只 include 它）
├── port.h            Port<Link, Key, State, Power>：UART + EN（KEY 或電源）+ STATE，讀一行、讀到安靜為止，結果碼；NoPin（模組共用的核心）
├── module/           實際買到的零件
│   ├── HC_05.h       HC05<Link, Key, State, Power>：EN、VCC、GND、TXD、RXD、STATE；上電後 EN HIGH 下 AT 命令（資料鮑率），結尾 \r\n；
│   │                 HC05PowerSwitch、HC05ByEn：EN 是電源開關的板子（沒有 KEY：沒連線時直接下 AT）
│   ├── HC_06.h       HC06<Link, State>：VCC、GND、TXD、RXD（STATE）；沒連線時下 AT 命令，沒有結尾，一秒一條
│   └── BT05.h        BT05<Link, State, Power>：BLE（FFE0 / FFE1）；沒連線時下 AT 命令，結尾 \r\n，沒回答會重送
├── tool/             建在它上面的常用功能
│   └── packet.h      Packet<Link, MAX_LENGTH, GAP_MS>：0xAA、LENGTH、COMMAND、DATA、CRC-8；crc8()
└── source/           電腦端
    ├── packet.py     同樣的封包：crc8()、encode()、Decoder；命令列收發（--port 序列埠或 --ble 名稱）
    └── ble.py        BleSerial：BLE 模組（FFE1）當序列埠用（read、write、in_waiting、close），bleak；--scan
```

## 介面需求

| 參數 | 需要的 static 函式 | 現成的實作 |
|---|---|---|
| `Link` | `init()`、`available()`、`read8(byte, timeoutMs)`、`write8(byte)`、`clearInput()`、`flush()`、`nowMs()`、`delayMs(ms)` | `Uno::UnoSoftSerial<RX_PIN, TX_PIN, BAUD, RX_SIZE>`、`Uno::UnoSerial<BAUD, RX_SIZE, TX_SIZE>` |
| `Key`、`Power` | `init()`、`write(high)` | `Uno::UnoOutputPin<PIN>`、`bluetooth::NoPin` |
| `State` | `init()`、`read()` | `Uno::UnoInputPin<PIN>`、`bluetooth::NoPin` |
| `Packet` 的 `Link` | `init()`、`available()`、`read8(byte, timeoutMs)`、`write8(byte)`、`nowMs()` | `HC05`、`HC06`、`Port`、`Uno::UnoSerial<BAUD>` |

`Port<Link, …>` 繼承 `Link`，所以 `Link` 有的函式（`readFlash8`、`printText`…）`Port` 都有：它本身就是 Link，可以給 `stream::Puller`、`Packet`。

`NoPin`：`IS_NO_PIN = true`，`init()`、`write()` 什麼都不做，`read()` 回傳 false；`IsNoPin<Pin>::value` 判斷。

## 函式

`Port<Link, Key, State>`（`port.h`）：

| 函式 | 說明 |
|---|---|
| `init()` | 設定 Key（LOW：資料模式）、State（輸入）、Link |
| `available()`、`read8()`、`write8()`、`clearInput()`、`nowMs()`、`delayMs()`… | 來自 `Link` |
| `writeText(text)` | 送出文字的每個位元組（不含 `'\0'`） |
| `writeNumber(value)` | 送出十進位數字 |
| `write(data, length)` | 送出 length 個位元組 |
| `isConnected()` | 每次讀一次 STATE，回傳濾過的連線狀態：持續 HIGH `CONNECTED_MS` 才 true，持續 LOW `DISCONNECTED_MS` 才 false（沒有 `State` 是編譯錯誤） |
| `readState()` | 回傳 STATE 現在的電位，沒有濾波（沒有 `State` 是編譯錯誤） |
| `setCommandMode(on)` | 先 `flush()`（送緩衝區裡的資料不能變成命令），再設定 EN / KEY：HIGH = AT 命令模式（沒有 `Key` 是編譯錯誤）；不會等模組反應 |
| `isCommandMode()` | 回傳 Key 現在是不是 HIGH |
| `readLine(buffer, size, timeoutMs)` | 讀一行到 `'\n'`（去掉 `\r`、`\n`，加 `'\0'`，超過 size - 1 的截掉）；時間內沒有 `'\n'` 回傳 false |
| `readUntilQuiet(buffer, size, firstMs, quietMs)` | 最多等 firstMs 等第一個位元組，再讀到 quietMs 沒有新的為止；回傳留下的位元組數 |
| 常數 `HAS_KEY`、`HAS_STATE` | 有沒有接這兩隻腳 |
| 常數 `CONNECTED_MS`、`DISCONNECTED_MS` | `isConnected()` 的濾波時間 |
| 常數 `OK`、`ERROR_NO_ANSWER`、`ERROR_REFUSED`、`ERROR_BAD_VALUE`、`ERROR_CONNECTED` | AT 函式的結果（0 ～ 4） |

`HC05<Link, Key, State>`（`module/HC_05.h`，繼承 `Port`）：

| 函式 | 說明 |
|---|---|
| `init()` | `Port::init()`，記下時間：第一條命令會等到上電後 `BOOT_MS` |
| `check()` | 送 `AT`，等 `OK` |
| `setName(name)` | 送 `AT+NAME=name` |
| `setPin(pin)` | 送 `AT+PSWD=1234`；被拒絕再送 `AT+PSWD="1234"`（韌體 3.0） |
| `setBaud(baud)` | 送 `AT+UART=baud,0,0`（下次啟動生效） |
| `setRole(role)` | 送 `AT+ROLE=n` |
| `version(reply, size)` | 送 `AT+VERSION?`，把版本（去掉 `+VERSION:`）放進 reply |
| `restart()` | 送 `AT+RESET`，EN 設成 LOW，等 `RESTART_MS`，清掉收到的東西 |
| `command(text, reply, size, timeoutMs)` | 送任何命令（加 `\r\n`）；第一行不是 OK 的回答（去掉 `+XXX:`）放進 reply |

每條命令：沒在命令模式就 `flush()`、EN HIGH、等 `KEY_SETTLE_MS` → `clearInput()` → 送命令 + `\r\n`、`flush()` → 一行一行讀到 `OK`（`OK`）、`ERROR…` / `FAIL`（`ERROR_REFUSED`）或時間到（`ERROR_NO_ANSWER`）→ 原本不在命令模式就 EN LOW。先呼叫 `setCommandMode(true)` 可以連續送好幾條，EN 不會一直上下。

`HC06<Link, State>`（`module/HC_06.h`，繼承 `Port<Link, NoPin, State>`）：

| 函式 | 說明 |
|---|---|
| `init()` | `Port::init()`，記下時間：第一條命令會等到 `GAP_MS` 之後 |
| `check()` | 送 `AT`，回答以 `OK` 開頭才是 `OK` |
| `setName(name)` | 送 `AT+NAMEname`（回答 `OKsetname`） |
| `setPin(pin)` | 送 `AT+PIN1234`（回答 `OKsetPIN`） |
| `setBaud(baud)` | 送 `AT+BAUDn`（1 ～ 9、A ～ C；回答 `OK9600` 之類） |
| `version(reply, size)` | 送 `AT+VERSION`，回答（去掉開頭的 `OK`）放進 reply |
| `command(text, reply, size)` | 原樣送出；有回答且不是 `ERROR…` 開頭就是 `OK`，整個回答放進 reply |

每條命令：有 `State` 且連線中就回傳 `ERROR_CONNECTED` → `flush()`（還沒送完的資料會黏進命令）→ 等到離上一條命令開始 `GAP_MS` → `clearInput()` → 送出（沒有結尾）→ 最多等 `REPLY_MS` 等第一個位元組，再讀到 `QUIET_MS` 沒有新的。

`BT05<Link, State, Power>`（`module/BT05.h`，繼承 `Port<Link, NoPin, State, Power>`）：

| 函式 | 說明 |
|---|---|
| `init()` | `Port::init()`（Power HIGH：模組開），記下時間：第一條命令會等到 `BOOT_MS` 之後 |
| `check()` | 送 `AT`，等 `OK` |
| `setName(name)` | 送 `AT+NAMEname`（回答 `+NAME=name`、`OK`；不能有 `=`，不然名稱會變成 `=name`） |
| `name(reply, size)` | 送 `AT+NAME`，名稱放進 reply |
| `setPin(pin)` | 送 `AT+PIN123456`（剛好 `PIN_LENGTH` 位數字） |
| `setBaud(baud)` | 送 `AT+BAUDn`（HC-06 的代碼；4 = 9600 實測過） |
| `version(reply, size)` | 送 `AT+VERSION`，版本（`v6.1`）放進 reply |
| `address(reply, size)` | 送 `AT+LADDR`，藍牙位址放進 reply |
| `restart()` | 送 `AT+RESET`，等 `RESTART_MS` |
| `command(text, reply, size)` | 送任何命令（加 `\r\n`）；第一個 `+XXX=value` 的 value 放進 reply；`OK` 或之後沒有 `OK` 的那行就是 `OK` |

每條命令：等到開機後 `BOOT_MS`、離上一條回答 `GAP_MS`（這段時間一直看 STATE）→ 有 `State` 且連線中就回傳 `ERROR_CONNECTED` → `flush()` → 送命令 + `\r\n` → 一行一行讀：`OK`（`OK`）、`ERROR`（`ERROR_REFUSED`）、`+XXX=value`（問的命令到這裡就結束）→ 完全沒有回答就等 `RETRY_MS` 再送，總共 `TRIES` 次。

`Port::setPower(on)`：Power HIGH = 模組開（重新開機，約一秒），LOW = 關（沒有 `Power` 是編譯錯誤）。`HC05` 沒有 `Key` 時：AT 命令直接送（連線中有 `State` 就回傳 `ERROR_CONNECTED`）。

`Packet<Link, MAX_LENGTH, GAP_MS>`（`tool/packet.h`）：

| 函式 | 說明 |
|---|---|
| `init()` | `Link::init()`，清掉狀態和 `dropped()` |
| `update()` | 讀所有已經到的位元組（不會等）；收到一整個 CRC 正確的封包就回傳 true（剩下的下一次再讀） |
| `command()`、`length()`、`data()` | 回傳上一個封包；到下一次 `update()` 前有效 |
| `readUint8(i)`、`readInt8(i)`、`readUint16(i)`、`readInt16(i)` | 回傳 DATA 第 i 個位元組起的數字（低位在前）；超過長度是 0 |
| `dropped()` | 回傳 `init()` 後丟掉的封包數（到 65535 為止） |
| `send(command, data, length)` | 送一個封包（等到全部送出） |
| `sendText(command, text)` | 把文字（不含 `'\0'`，最多 255）當 DATA 送出 |

## 參數

| 參數 | 預設 | 說明 |
|---|---|---|
| `Link` | — | `Uno::UnoSerial<BAUD, RX_SIZE, TX_SIZE>`：`BAUD` = 模組的資料鮑率（HC-05、HC-06 出廠是 9600）；`RX_SIZE` 至少放得下 `loop()` 最忙那段時間收到的 byte |
| `Key` | `Port`：`NoPin`；`HC05`：必填 | EN / KEY 的輸出腳 |
| `State` | `NoPin` | STATE 的輸入腳 |
| `MAX_LENGTH`（`Packet`） | 32 | 收得下的最長 DATA（RAM） |
| `GAP_MS`（`Packet`） | 100 | 封包中兩個位元組隔超過這麼久就丟掉這個封包 |

模組裡的常數（依 datasheet 和實測估的，要調就改標頭檔）：

| 常數 | 說明 |
|---|---|
| `HC05::BOOT_MS` | 上電後多久才收 AT 命令 |
| `HC05::KEY_SETTLE_MS` | EN HIGH 後等多久才送命令 |
| `HC05::REPLY_MS` | 最多等 `OK` 多久（`command()` 可以另外給） |
| `HC05::RESTART_MS` | `restart()` 後等多久 |
| `HC06::GAP_MS` | 兩條命令開始的間隔（datasheet：不要超過 1 Hz） |
| `HC06::REPLY_MS`、`QUIET_MS` | 等第一個位元組多久；多久沒有新的算回答結束 |
| `MAX_NAME_LENGTH` | 名稱最長幾個字元（HC-06 datasheet；HC-05 用一樣的） |

## Datasheet 重點

來源：HC Serial Bluetooth Products User Instructional Manual（廣州匯承 HC / Linvor），和 HC-06 底板的說明。

| 項目 | HC-05 | HC-06 |
|---|---|---|
| 模組本身 | VCC 3.0 ～ 4.2 V（標準 3.3 V）；底板穩壓 3.6 ～ 6 V | 一樣 |
| 電流 | 配對中 30 ～ 40 mA（平均約 25 mA），連線後 8 mA，沒有睡眠模式 | 未配對約 30 mA，配對後約 10 mA |
| UART_RXD | 沒有上拉電阻：要外加 | 一樣 |
| 角色 | 主 / 從可用 AT 切換 | 出廠就決定，不能換 |
| KEY | PIN34：上電時 HIGH → AT 模式 2（38400，全部命令）；上電後 HIGH → AT 模式 1（資料鮑率，部分命令）；連線中也能進 AT，LOW 回資料模式 | PIN26：只有主機用來清除記憶的配對；從機沒有作用 |
| 指示 | LED1（PIN31）：1 Hz = AT 模式 2，2 Hz = 等配對或 AT 模式 1，一秒閃兩下 = 已連線；LED2（PIN32 = STATE）：配對前 LOW、後 HIGH | LED（PIN24）：從機配對前 102 ms 方波，配對後 HIGH |
| 命令結尾 | `\r\n` | 沒有結尾，靠時間間隔：一秒不要超過一條 |
| AT 什麼時候有效 | KEY HIGH 時（不管有沒有連線） | 配對前（配對後是透明傳輸） |
| 預設 | 名稱 HC-05、PIN 1234、9600、從機 | 名稱 linvor（新的是 HC-06）、PIN 1234、9600 |
| 鮑率 | 4800 ～ 1.3M | 1200 ～ 1.3M |
| 其他 | | 新 PIN 要模組完全放電後才生效；名稱最多 20 字元 |

## 為什麼這樣設計

- **BT05 的命令是在實物上量出來的**（韌體 v6.1，`AT+HELP` 列出全部）：和 HC-05 的 `AT+NAME=`、`AT+PSWD=` 不同，送錯格式不一定回 `ERROR`（`AT+NAME=x` 會把名稱設成 `=x`），所以每種模組有自己的 module 檔，不共用一套命令。剛改完名稱模組會暫時不理命令（量到：緊接的 `AT+PIN` 沒回答、稍後再送就好），所以沒有回答時重送。
- **EN 可能是電源開關**：量過的 BT05 板子 EN HIGH = 開、LOW = 關、不驅動 = 維持原狀（沒有上拉、下拉）；當 KEY 用時一開機拉 LOW 會讓模組一直關著（沒有廣播、TXD LOW、什麼都不回答）。所以 `Power` 和 `Key` 分開，`3_diagnose.cpp` 量 EN 不驅動 / LOW / HIGH 時 TXD 的電位來分辨。
- **BLE 和 SPP 放同一個資料夾**：板子這邊只看到 UART，命令、封包、接線都一樣；不同的只有電腦端（`ble.py` 把 FFE1 包成和 pyserial 一樣的 read / write，所以 `packet.py`、`round_trip.py` 只多一個 `--ble`）。
- **STATE 用時間濾波**：3.3 V 進 5 V 的 UNO 只剩 0.3 V 餘裕；有些板子 STATE 浮接，有些 HC-06 板子的 STATE 是 LED 的複本，沒連線時會閃（datasheet：從機配對前是 102 ms 週期的方波）。`CONNECTED_MS` 比所有閃爍的 HIGH 都長（HC-05 最慢的 AT 模式閃爍 HIGH 500 ms），所以閃的永遠不算連線；`DISCONNECTED_MS` 擋掉短的突波。不用中斷：每次 `isConnected()` 讀一次，程式本來就一直在問。
- **`HC06` 等 `GAP_MS` 時一直看 STATE**：濾波需要歷史；`GAP_MS` ≥ `CONNECTED_MS`（編譯時檢查），所以每條命令前都至少看過 `CONNECTED_MS` 的 STATE。

- **AT 用資料鮑率（上電後才拉 EN）**：`Uno::UnoSerial<BAUD>` 的鮑率在編譯時決定；用 38400 的 AT 模式 2 要上電時就拉 EN，還要能控制模組的電源，UART 還得換速度。代價是少數命令（如 `AT+NAME?`、`AT+INQ`）在模式 1 可能不回答。
- **`Port` 繼承 `Link`**：藍牙只是換了一條線，原本吃 Link 的東西（`stream::Puller`）不用改。
- **`NoPin` 而不是腳位號碼 `Uno::NONE`**：驅動程式不能用 `Uno` 的東西（不依賴其他函式庫）；和 `L298N::Ground` 同樣的作法。
- **CRC-8（多項式 0x07、起始 0，即 CRC-8/SMBUS，`"123456789"` 的檢查值 0xF4）**：擋住所有 1 ～ 2 位元錯誤、所有 8 位元以內的連續錯誤，其他錯誤漏掉的機率 1/256；加總會漏掉兩個剛好抵銷的錯。逐位元算，AVR 上每個位元組約 50 個週期，不用 256 bytes 的表。
- **中斷在另一個 header（`Uno_serial_buffer.h`）**：放在 `Uno_serial.h` 會讓每個用序列埠的程式多約 330 bytes Flash；分開後只有用緩衝區的程式付。忘了 include 是編譯錯誤，不會跑到沒有處理函式的中斷而重開機。
- **送 AT 前先 `flush()`**：有送出緩衝區時，資料還在背景送；這時拉 EN 或送 HC-06 的命令，資料會被當成命令的一部分。
- **數字低位在前**：AVR 和 x86、ARM 的記憶體順序，`packet.py` 用 `struct` 的 `<`。

## 輸入限制

| 項目 | 範圍 | 超出時 |
|---|---|---|
| 名稱 | 1 ～ `MAX_NAME_LENGTH` 個可印字元（空白 ～ `~`） | 回傳錯誤（`ERROR_BAD_VALUE`） |
| PIN | 剛好 4 位數字 | 回傳錯誤（`ERROR_BAD_VALUE`） |
| HC-05 的 `setBaud` | 4800、9600、19200、38400、57600、115200、230400、460800、921600、1382400 | 回傳錯誤（`ERROR_BAD_VALUE`） |
| HC-06 的 `setBaud` | 1200、2400 和 HC-05 的清單 | 回傳錯誤（`ERROR_BAD_VALUE`） |
| `setRole` | 0 ～ 2 | 回傳錯誤（`ERROR_BAD_VALUE`） |
| `HC05` 的 `Key` | 一隻腳（不是 `NoPin`） | 編譯錯誤 |
| 沒有 `State` 呼叫 `isConnected()`；沒有 `Key` 呼叫 `setCommandMode()` | | 編譯錯誤 |
| `Packet` 的 `MAX_LENGTH` | 1 ～ 255 | 編譯錯誤 |
| 收到的 LENGTH | 0 ～ `MAX_LENGTH` | 丟掉（`dropped()` + 1） |
| `HC05` 的一行回答 | 39 個字元 | 截掉 |
| `HC06` 的回答（`check`、`set…`、`version`） | 23 個字元 | 截掉 |
| `version`、`command` 的 reply | `size` - 1 個字元 | 截掉 |

## 資源

| 項目 | 用量 |
|---|---|
| RAM | `UnoSerial` 的緩衝區：`RX_SIZE` + `TX_SIZE` + 14 bytes；`Port`：7 bytes（命令模式、STATE 的濾波）；`HC05`、`HC06`：再 4 bytes；AT 命令時堆疊約 40 ～ 60 bytes；`Packet`：`MAX_LENGTH` + 11 bytes |
| Flash | 測試程式整支約 2.6 ～ 3 KB（含 AT 命令的字串） |
| 硬體 | `UnoSoftSerial`：兩隻腳，RX 那組的腳位改變中斷（每個邊緣約 5 µs）；或 `UnoSerial`：D0 / D1，有緩衝區時 USART_RX、USART_UDRE 中斷（每個位元組約 3 µs）；EN、STATE 各一隻腳（可不接） |
| 時間 | AT 命令會等（見前提條件）；`Packet::update()` 不會等；`send()` 有送出緩衝區時放得下就馬上回來，沒有時每個位元組在 9600 鮑約 1 ms |
