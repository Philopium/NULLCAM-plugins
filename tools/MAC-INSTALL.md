# PLUGIN_NAME — installazione su macOS

È un effetto **VST3** per Ableton Live: lo metti su una traccia audio come
 qualsiasi altro plugin.

Nello zip c'è la cartella **PLUGIN_NAME.vst3**. Va copiata **intera**, non aperta.

Il binario è **universale**: funziona sia su Mac con chip Apple Silicon
(M1/M2/M3/M4) sia su Mac Intel.

---

## Installazione (30 secondi, senza Terminale)

1. Scompatta lo zip (double click su `PLUGIN_NAME-macOS-VST3.zip`).
2. Apri il **Finder** e premi **⌘ + Shift + G** (Vai → Vai alla cartella).
3. Incolla questo percorso e premi Invio:

   ```
   ~/Library/Audio/Plug-Ins/VST3
   ```

   Se la cartella `VST3` non esiste, creala con **⌘ + Shift + N**.
4. Trascina la cartella **PLUGIN_NAME.vst3** dentro quella finestra.
5. Apri **Ableton Live**: *Impostazioni → Plug-In → Riscansiona*.
6. Nel browser, sotto **Plug-in → VST3**, cerca **PLUGIN_NAME** e trascinalo
   su una traccia.

Per installarlo per **tutti gli utenti** del Mac (serve una password di
amministratore), la cartella è `/Library/Audio/Plug-Ins/VST3`.

---

## Se Ableton non lo vede, o dice che il plugin è "danneggiato"

Il plugin non è firmato con un certificato Apple (per quello serve un account
Apple Developer a pagamento), quindi macOS a volte lo blocca. Si risolve con
**una riga nel Terminale**:

1. Apri **Terminale** (Applicazioni → Utility → Terminale).
2. Incolla e premi Invio:

   ```bash
   xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/"PLUGIN_NAME.vst3"
   ```

3. Torna in Ableton e fai **Riscansiona**.

Se invece preferisci non usare il Terminale: carica il plugin una volta in
Ableton, poi apri **Impostazioni di Sistema → Privacy e Sicurezza**, scorri
in fondo e premi **Apri comunque** sul messaggio che parla di NULLCAM.

---

## I preset

I tuoi preset sono file XML in questa cartella:

```
~/Library/Application Support/NULLCAM/PLUGIN_NAME/presets
```

Sono file di testo: puoi copiarli, fare un backup o mandarli a un amico.
Nello zip trovi anche il file `LEGGIMI - installazione macOS.md` con i dettagli
su come si usa il plugin.

Buon divertimento!
