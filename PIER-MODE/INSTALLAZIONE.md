# PIER-MODE — installazione

È un effetto VST3: prende l'audio in ingresso e ci applica sopra la catena di
danneggiamento dell'app NULLCAM. Dentro Ableton lo metti su una traccia audio, come
un qualsiasi altro effetto.

Nello zip c'è la cartella **PIER-MODE.vst3**. Va copiata intera, non aperta.

## macOS

1. Copia `PIER-MODE.vst3` in `~/Library/Audio/Plug-Ins/VST3`
   (dal Finder: *Vai → Vai alla cartella*, incolla il percorso). Se la cartella `VST3`
   non c'è, creala. Per installarlo per tutti gli utenti: `/Library/Audio/Plug-Ins/VST3`.
2. Il plugin **non è firmato**, quindi al primo caricamento macOS lo blocca. Apri il
   Terminale e togli il blocco:

   ```bash
   xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/"PIER-MODE.vst3"
   ```

   In alternativa: prova a caricarlo in Ableton, poi *Impostazioni di Sistema →
   Privacy e Sicurezza*, scorri in fondo e premi **Apri comunque** sul messaggio che
   parla di NULLCAM.
3. Se il Mac è Apple Silicon, lo zip contiene il binario universale: funziona sia su
   Apple Silicon sia su Intel.

## Windows

1. `build-windows.bat` copia già il plugin: in `C:\Program Files\Common Files\VST3`
   se la console ha i permessi di amministratore, altrimenti in
   `%LOCALAPPDATA%\Programs\Common\VST3` (nessun permesso speciale richiesto).
   In Ableton: *Impostazioni → Plug-In*, aggiungi quella cartella come cartella VST3
   personalizzata se non è quella di sistema, poi **Riscansiona**.
2. Se preferisci comunque installarlo per tutti gli utenti, apri il Prompt dei comandi
   come amministratore ed esegui:

   ```bat
   xcopy /E /I /Y "build\NullcamChain_artefacts\Release\VST3\PIER-MODE.vst3" "C:\Program Files\Common Files\VST3\PIER-MODE.vst3"
   ```
3. Se Windows mostra l'avviso "Windows ha protetto il PC", scegli **Ulteriori
   informazioni → Esegui comunque**. In alternativa: tasto destro sullo zip scaricato →
   *Proprietà* → spunta **Annulla blocco** → OK, prima di scompattarlo.

## Ableton Live

1. *Impostazioni → Plug-In*: attiva **Usa cartelle di sistema VST3** (o aggiungi la
   cartella dove l'hai messo) e premi **Riscansiona**.
2. Nel browser, sotto **Plug-in → VST3**, cerca **PIER-MODE** e trascinalo su una
   traccia audio.
3. Se non compare: riavvia Live dopo la riscansione, ed evita di mettere il plugin
   dentro cartelle con permessi strani (Desktop protetto, iCloud Drive).

## Come si usa

Il pannello è una pedaliera: cinque schede in fila, nell'ordine in cui il segnale le
attraversa, `BAND → CRUSH → ECHO → RING → OUT`. Sotto ogni manopola c'è il valore con
la sua unità (`330 Hz`, `344 ms`, `98%`): trascina in su e in giù (o usa la rotellina)
per spostarlo, **doppio clic** per scrivere un numero esatto e premere Invio. La
scheda CRUSH mostra in grande i bit e il sample & hold che stai ottenendo.

Parte già con il suono **MOTION** dell'app, e la targhetta in alto dice quale preset
stai ascoltando (con un `*` se hai mosso qualcosa):

- **MOTION** — il suono dell'effetto motion extraction
- **SORT** — più crushato, senza eco
- **CLEAN** — tutto aperto e pulito, per partire da zero

Manopole, nell'ordine in cui il segnale le incontra: `HPF` e `LPF` (la banda che
decide il carattere), `CRUSH` (bit e sample rate), `DELAY` ed `ECHO` (l'eco),
`FEEDBACK` (quanto durano i rimbalzi), `RING Hz` e `RING` (il ronzio metallico),
`DRY` (quanto secco resta nel mix), `OUTPUT` (volume finale).

## Preset tuoi

La barra in alto ha `LOAD PRESET`, **SAVE** e **DEL**. Con SAVE dai un nome alle
manopole così come sono e il preset viene scritto come file qui:

- **Windows** — `%APPDATA%\NULLCAM\PIER-MODE\presets`
- **macOS** — `~/Library/Application Support/NULLCAM/PIER-MODE/presets`

Sono file XML: puoi copiarli su un'altra macchina, metterli in un backup o mandarli a
qualcuno. DEL cancella il preset selezionato dal disco. I preset dell'app (MOTION,
SORT, CLEAN) restano sempre, non si possono cancellare.

Consigli pratici: alza `OUTPUT` piano, perché eco e ronzio si sommano al secco;
`FEEDBACK` oltre 0.95 diventa ingestibile in fretta; se il suono è troppo "telefono",
abbassa `HPF` verso 100–150 Hz. Su una voce funziona meglio dell'app perché senti il
risultato in tempo reale.
