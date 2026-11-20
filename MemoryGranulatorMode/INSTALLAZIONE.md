# MemoryGranulatorMode — installazione e uso

## Cosa contiene lo zip Windows

```text
MemoryGranulatorMode.vst3             il plugin VST3
LEGGIMI - installazione.md              questo file
LICENZA - AGPLv3.md                     licenza del software
```

La cartella `MemoryGranulatorMode.vst3` va copiata **intera**: non aprire il
bundle e non copiare soltanto il file interno.

---

## Windows

1. Copia `MemoryGranulatorMode.vst3` in una delle cartelle VST3:

   - `C:\Program Files\Common Files\VST3` — per tutti gli utenti;
   - `%LOCALAPPDATA%\Programs\Common\VST3` — solo per te, senza diritti di
     amministratore.

   Se esiste già una versione di `MemoryGranulatorMode.vst3`, sostituiscila:
   non serve disinstallare nulla.

2. Riscansiona i plugin nella DAW:

   - **Ableton Live**: `Preferenze > Plug-in > Riscansiona`. Se hai usato la
     cartella per-utente, aggiungi
     `%LOCALAPPDATA%\Programs\Common\VST3` in
     `Plug-in > "cartella VST3 personalizzata"`.
   - **Reaper**: `Opzioni > Preferenze > Plugin > VST: imposta la cartella VST3
     e clicca "Aggiorna"`.
   - **Cubase / Studio One**: apri e chiudi la scansione dei plugin, oppure
     riavvia la DAW.

3. Cerca **MemoryGranulatorMode** nella lista degli effetti e inseriscilo su
   una traccia audio.

Il build automatico `build-windows.bat` copia già il bundle nella cartella
VST3 per-utente e prepara anche lo zip.

---

## macOS

Il bundle macOS è **universale**: funziona sia su Mac con chip Apple Silicon
(M1/M2/M3/M4) sia su Mac Intel.

1. Copia `MemoryGranulatorMode.vst3` dentro
   `~/Library/Audio/Plug-Ins/VST3`:

   ```bash
   mkdir -p ~/Library/Audio/Plug-Ins/VST3
   cp -R MemoryGranulatorMode.vst3 ~/Library/Audio/Plug-Ins/VST3/
   ```

2. Se macOS blocca il plugin, il binario è firmato in modo *ad-hoc* ma non
   notarizzato. Rimuovi la quarantena:

   ```bash
   xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/"MemoryGranulatorMode.vst3"
   ```

3. Riscansiona i plugin nella DAW. In Ableton:
   `Preferenze > Plug-in > Riscansiona`.

Per installarlo per tutti gli utenti serve la cartella
`/Library/Audio/Plug-Ins/VST3` e una password di amministratore.

---

## Instradamento MIDI in Ableton Live

MemoryGranulatorMode è un effetto VST3 con **ingresso audio e ingresso MIDI**.
La nota non crea il suono: il suono nasce dal materiale registrato nel
plugin; MIDI controlla i grani.

### Configurazione consigliata

1. Crea una **traccia MIDI** per le note.
2. Nella sezione `Input` in alto della traccia MIDI scegli la sorgente:
   controller, tastiera o porta MIDI.
3. Crea una **traccia audio**, inserisci `MemoryGranulatorMode` e invia un
   segnale alla sua ingressione audio.
4. Nella sezione `Output` della traccia MIDI apri il selettore `MIDI To`,
   scegli la traccia audio che contiene il plugin e seleziona
   `MemoryGranulatorMode` nel chooser del canale MIDI.
5. Nel plugin imposta `MIDI CHANNEL = OMNI`. Il routing interno di Live può
   infatti fondere i canali MIDI in un solo canale.
6. Scegli `PITCH SOURCE`:

   - `MIDI` — la nota MIDI guida direttamente il pitch dei grani;
   - `AUDIO` — il plugin traccia il pitch monofonico del segnale audio;
   - `HYBRID` — MIDI resta l'ancoraggio musicale, l'audio muove le mappature
     secondarie.

7. Attiva `ARM` e poi `RELEASE` nella modalità `CAPTURE+RELEASE`, oppure
   lascia `CAPTURE` attivo in modalità `LIVE`, per decidere quando il
   materiale viene scritto nella memoria.

### Se non compare nel chooser MIDI

La disponibilità del routing diretto a un effetto audio può variare fra le
versioni di Live. Controlla che:

- la traccia MIDI sia attivata e non sia in modalità `MIDI Off`;
- `MIDI To` sia impostato sulla traccia che contiene il plugin;
- il plugin sia caricato **prima** di aprire il chooser MIDI;
- `MIDI CHANNEL` sia `OMNI`.

In alternativa, usa il percorso MIDI/audio equivalente supportato dalla tua
versione di Live. Il plugin riceve eventi VST3 MIDI autentici: Note On/Off,
velocity, CC, aftertouch e pitch wheel.

---

## Primi passi

1. Metti il plugin su una traccia audio e fai entrare un suono: una voce, un
   synth o una breve registrazione.
2. Parti da `MEMORY BLOOM`. Con `CAPTURE` attivo il materiale entra nella
   memoria circolare.
3. Alza `DENSITY` e abbassa `GRAIN SIZE` per sentire i grani separati. Per
   una nuvola continua, alza `OVERLAP`.
4. Attiva `FREEZE`: la memoria smette di cambiare, ma i grani continuano a
   nascere dal materiale congelato.
5. Ora instrada una nota MIDI e passa `PITCH SOURCE` a `MIDI`. La stessa
   memoria suona ad altezze diverse.
6. `SCAN SPEED` muove il playhead e le sorgenti dei grani; non cambia il
   pitch. È volutamente indipendente.
7. Alza `MEM FB` per far evolvere la memoria. Con `OVERDUB` il nuovo materiale
   si somma al precedente; con `HOLD` l'ingresso non viene più catturato ma
   il feedback può ancora riscrivere il contenuto.
8. `DRY / WET` a zero lascia passare soltanto il dry; a uno senti soltanto
   grani e riverberazione.

---

## Preset

Factory preset inclusi:

- `MEMORY BLOOM`
- `CHORD ECHO`
- `FROZEN VERB`
- `OVERDUB CLOUD`
- `HARMONIC WEATHER`
- `POLY RHYTHM`
- `RESONANT CIRCUIT`
- `DARK ARCHIVE`

I tuoi preset utente sono file XML in:

```text
Windows   %APPDATA%\NULLCAM\MEMORY-GRANULATOR-MODE\presets
macOS     ~/Library/Application Support/NULLCAM/MEMORY-GRANULATOR-MODE/presets
```

Nel pannello, `SAVE` salva il preset e `DEL` lo cancella. Puoi anche copiare
i file XML a mano: sono leggibili e sopravvivono a una reinstallazione.

Il progetto della DAW salva automaticamente i 99 parametri, indipendentemente
dai file preset.

---

## Problemi

**Non compare nella lista dei plugin.**

Copia la cartella `MemoryGranulatorMode.vst3` intera, riavvia o riscansiona
la DAW e controlla che la cartella VST3 sia quella giusta. Su macOS elimina
anche la quarantena con il comando `xattr` sopra.

**Sento soltanto l'ingresso e nessun grano.**

La memoria deve contenere materiale. Controlla `CAPTURE`, `REC LEVEL` e che
l'ingresso audio stia effettivamente arrivando al plugin. Poi prova
`FREEZE` dopo un secondo di cattura.

**Le note MIDI non cambiano nulla.**

Controlla `PITCH SOURCE = MIDI`, `MIDI CHANNEL = OMNI`, l'intervallo
`NOTE LOW / NOTE HIGH` e il routing `MIDI To` della traccia MIDI. Nel pannello
la card `ACTIVE NOTES` mostra le note realmente ricevute.

**Il tracking audio salta spesso.**

Il tracker è monofonico e ha bisogno di una fondamentale chiara. Su uno
strumento polifonico usa `MIDI` o `HYBRID`. Abbassa `TRACK LOW`; alza
`TRACK HIGH` solo se la ricerca deve escludere frequenze basse e regola
`CONFIDENCE`.

**Il feedback diventa rumoroso.**

È il comportamento voluto di un loop ad alto guadagno, ma non deve mai
esplodere. Riduci `MEM FB`, `REVERB FEEDBACK`, `FB DRIVE` oppure `FB STABILITY`.
L'uscita resta limitata e finita.

**Cambiando preset la memoria non si svuota.**

I preset cambiano i parametri, non cancellano il materiale già registrato.
Premi `CLEAR` per azzerare memoria, playhead e grani attivi.

---

## Licenza e codice sorgente

MemoryGranulatorMode è software libero: puoi usarlo, copiarlo e modificarlo
secondo la **GNU Affero General Public License v3** (o una versione
successiva). Il plugin è distribuito **senza garanzia** di alcun tipo.

Il codice sorgente completo corrispondente a questo binario è pubblico qui:

```text
https://github.com/Philopium/NULLCAM-plugins
```

Licenza completa:
`https://www.gnu.org/licenses/agpl-3.0.html`

JUCE è dual-licensed: per la distribuzione open source vale la AGPLv3; una
distribuzione proprietaria o commerciale richiede una licenza JUCE
commerciale.
