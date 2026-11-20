# EchoMatrixMode - installazione

## Cosa contiene lo zip

```
EchoMatrixMode.vst3             il plugin
LEGGIMI - installazione.md       questo file
```

## Windows

1. Copia la cartella `EchoMatrixMode.vst3` dentro una delle cartelle VST3:

   - `C:\Program Files\Common Files\VST3` (per tutti gli utenti)
   - `%LOCALAPPDATA%\Programs\Common\VST3` (solo per te, senza diritti di admin)

   Se la cartella `EchoMatrixMode.vst3` esiste già, **sostituiscila** con quella
   nuova: non serve disinstallare nulla.

2. Riscansiona i plugin nel tuo DAW.

   - **Ableton Live**: `Preferenze > Plug-in > Riscansiona`. Se usi la cartella
     per-utente, aggiungi `%LOCALAPPDATA%\Programs\Common\VST3` in
     `Plug-in > "cartella VST3 personalizzata"`.
   - **Reaper**: `Opzioni > Preferenze > Plugin > VST: imposta la cartella VST3 e
     clicca "Aggiorna"`.
   - **Cubase / Studio One**: apri e chiudi la scansione dei plugin (o riavvia la DAW).

3. Cerca **EchoMatrixMode** nella lista degli effetti e inseriscilo su una traccia.

## macOS

1. Copia `EchoMatrixMode.vst3` dentro `~/Library/Audio/Plug-Ins/VST3`:

   ```bash
   mkdir -p ~/Library/Audio/Plug-Ins/VST3
   cp -R EchoMatrixMode.vst3 ~/Library/Audio/Plug-Ins/VST3/
   ```

2. Se macOS blocca il plugin ( è firmato in modo "ad-hoc" ), rimuovi la quarantena:

   ```bash
   xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/"EchoMatrixMode.vst3"
   ```

3. Riscansiona i plugin nella DAW (in Ableton: `Preferenze > Plug-in > Riscansiona`).

## Primi passi

1. Metti il plugin su una traccia: con `DOTTED EIGHTH` senti subito un ritorno
   ritardato di 375 ms.
2. Il pattern è di 16 passi e i 16 passi dividono **un periodo di delay**: il
   passo 1 ripete esattamente un tempo di delay dopo l'ingresso, il passo 16
   quasi due, e fra due passi ci sono `DELAY / 16`. I pad accesi sono i tap che
   generano ripetizioni: accendine qualcuno e ascolta come la grana si apre.
3. Clicca un pad per selezionarlo: a destra trovi `LEVEL` e `PAN` di quel passo,
   e le due righe sotto ti dicono **quando** suona quel passo (`REPEATS ... MS
   AFTER THE INPUT` e `TAP ... OF 16` dentro la finestra di delay).
   Ogni passo è una voce a sé, quindi un passo a sinistra e uno a destra si
   sentono davvero separati.
4. `FEEDBK` tiene vivo il riverbero, `MIX` decide quanto entra nel mix finale.
   Parti da `MIX` a metà e alza `FEEDBK` piano: con troppa saturazione (`DRIVE`)
   il loop diventa un casino, e va bene che sia così.
5. `SYNC` fa seguire il delay al tempo della DAW: con il trasporto fermo il tempo
   resta quello manuale.
6. `ACTIVE REPEATS` in basso a sinistra dice quante ripetizioni stanno suonando
   in questo momento, `PATTERN POSITION` mostra dove corre la testina del pattern.

## Preset

I tuoi preset stanno in:

```
Windows   %APPDATA%\NULLCAM\ECHOMATRIX-MODE\presets
macOS     ~/Library/Application Support/NULLCAM/ECHOMATRIX-MODE/presets
```

Un file `.xml` per preset, leggibile: puoi copiarlo, salvarlo o mandarlo a un amico.

Per cancellarli: il pulsante `DEL` nella barra in alto, oppure cancella il file.

## Problemi

**Non compare nella lista dei plugin.**
Controlla di aver copiato la cartella `EchoMatrixMode.vst3` intera (con `Contents`
dentro) e riavvia/riscansiona la DAW.

**Non sento niente.**
Controlla `MIX`: a zero esce solo il segnale pulito, senza riverbero. Controlla
anche che almeno un pad del pattern sia acceso.

**Le ripetizioni arrivano ma sembrano mescolate.**
È normale: le voci si sommano. Abbassa `FEEDBK` e `MIX` per sentire meglio la
grana, e usa `PAN` per separare i passi a destra.

**Il delay non è quello che mi aspetto in sync.**
Il sync usa il BPM che la DAW comunica: se il trasporto è fermo, il plugin resta
sul tempo manuale.

## Licenza e codice sorgente

EchoMatrixMode è software libero: puoi usarlo, copiarlo e modificarlo secondo la
**GNU Affero General Public License v3** (o una versione successiva). Il plugin è
distribuito **senza garanzia** di alcun tipo.

Il codice sorgente completo corrispondente a questo binario è pubblico qui:

```
https://github.com/Philopium/NULLCAM-plugins
```

(licenza completa: `https://www.gnu.org/licenses/agpl-3.0.html`)
