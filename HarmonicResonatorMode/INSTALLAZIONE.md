# HarmonicResonatorMode - installazione

## Cosa contiene lo zip

```
HarmonicResonatorMode.vst3             il plugin
LEGGIMI - installazione.md              questo file
```

## Windows

1. Copia la cartella `HarmonicResonatorMode.vst3` dentro una delle cartelle VST3:

   - `C:\Program Files\Common Files\VST3` (per tutti gli utenti)
   - `%LOCALAPPDATA%\Programs\Common\VST3` (solo per te, senza diritti di admin)

   Se la cartella `HarmonicResonatorMode.vst3` esiste già, **sostituiscila** con
   quella nuova: non serve disinstallare nulla.

2. Riscansiona i plugin nel tuo DAW.

   - **Ableton Live**: `Preferenze > Plug-in > Riscansiona`. Se usi la cartella
     per-utente, aggiungi `%LOCALAPPDATA%\Programs\Common\VST3` in
     `Plug-in > "cartella VST3 personalizzata"`.
   - **Reaper**: `Opzioni > Preferenze > Plugin > VST: imposta la cartella VST3 e
     clicca "Aggiorna"`.
   - **Cubase / Studio One**: apri e chiudi la scansione dei plugin (o riavvia la DAW).

3. Cerca **HarmonicResonatorMode** nella lista degli effetti e inseriscilo su una traccia.

## macOS

1. Copia `HarmonicResonatorMode.vst3` dentro `~/Library/Audio/Plug-Ins/VST3`:

   ```bash
   mkdir -p ~/Library/Audio/Plug-Ins/VST3
   cp -R HarmonicResonatorMode.vst3 ~/Library/Audio/Plug-Ins/VST3/
   ```

2. Se macOS blocca il plugin (è firmato in modo "ad-hoc"), rimuovi la quarantena:

   ```bash
   xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/"HarmonicResonatorMode.vst3"
   ```

3. Riscansiona i plugin nella DAW (in Ableton: `Preferenze > Plug-in > Riscansiona`).

## Primi passi

1. Metti il plugin su una traccia: con `BELL METAL` senti subito un suono
   metallico che suona più a lungo della nota.
2. `INTERACTION` **non è un wet/dry**: è la quantità di segnale che fa il giro
   del loop fra la distorsione e la banca di risonatori. Parti da metà e alzalo:
   quando è chiuso, il loop si autoalimenta e la nota continua a suonare anche
   dopo che hai smesso di suonarla.
3. `ROUTING` sceglie il percorso. `RES < > DIST` è il loop chiuso (il default):
   le armoniche della distorsione cadono sui partiali e i partiali tornano
   indietro. Con `DIST > RES` senti solo la distorsione che alimenta la banca.
4. `TUNING` sceglie la serie dei partiali: `HARMONIC` (220, 440, 660...),
   `ODD`, `EVEN`, `CHORD`, `METALLIC` (inharmonica, da campana).
5. `ALIGN` fa seguire alla banca la nota che stai suonando: con l'armonica
   `HARMONIC` la banca si sposta sulla fondamentale e la nota viene
   rinforzata.
6. La card `PARTIALS` in basso a sinistra non è un disegno: le barre sono i
   partiali veri del motore. La loro **posizione** è la frequenza, l'**altezza**
   è il livello in dB, e `1 OF 8 PARTIALS RINGING` dice quanti stanno suonando
   in questo momento.
7. `DRY / WET` decide quanto entra nel mix finale. A zero senti solo il
   segnale pulito.

## Preset

I tuoi preset stanno in:

```
Windows   %APPDATA%\NULLCAM\HARMONIC-RESONATOR-MODE\presets
macOS     ~/Library/Application Support/NULLCAM/HARMONIC-RESONATOR-MODE/presets
```

Un file `.xml` per preset, leggibile: puoi copiarlo, salvarlo o mandarlo a un amico.

Per cancellarli: il pulsante `DEL` nella barra in alto, oppure cancella il file.

## Problemi

**Non compare nella lista dei plugin.**
Controlla di aver copiato la cartella `HarmonicResonatorMode.vst3` intera (con
`Contents` dentro) e riavvia/riscansiona la DAW.

**Non sento niente.**
Controlla `DRY / WET`: a zero esce solo il segnale pulito, senza il trattamento.

**Il suono non è quello che mi aspetto.**
La banca di risonatori è molto selettiva: un partial suona solo se qualcosa
nella sua frequenza lo eccita. Con `HARMONIC` le armoniche della distorsione
cadono esattamente sui partiali, con `METALLIC` no: è il punto di quei due
preset. Abbassa `RESO` per sentire la banca più larga, oppure prova `TUBE CHOIR`
o `GLASS HARMONICS`.

**Il loop mi sfugge di mano / diventa un rumore.**
È normale: con `INTERACTION` e `FEEDBACK` alti il loop è chiuso e si
autoalimenta. Non è un bug, è il comando. Per tornare sotto controllo abbassa
`FEEDBK` o `FB DRIVE`, oppure apri il loop con `ROUTING > DIST > RES`. Il
livello resta sempre limitato.

**La coda viene "risweptata" quando finisce la nota.**
Dovrebbe già succedere: il tracker tiene la confidenza per qualche secondo. Se
lo senti, stai muovendo `TUNE` o `ALIGN` mentre la coda suona.

## Licenza e codice sorgente

HarmonicResonatorMode è software libero: puoi usarlo, copiarlo e modificarlo
secondo la **GNU Affero General Public License v3** (o una versione successiva).
Il plugin è distribuito **senza garanzia** di alcun tipo.

Il codice sorgente completo corrispondente a questo binario è pubblico qui:

```
https://github.com/Philopium/NULLCAM-plugins
```

(licenza completa: `https://www.gnu.org/licenses/agpl-3.0.html`)
