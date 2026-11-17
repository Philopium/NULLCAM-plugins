# SPAMPI-MODE - installazione

## Cosa contiene lo zip

```
SPAMPI-MODE.vst3             il plugin
LEGGIMI - installazione.md   questo file
```

## Windows

1. Copia la cartella `SPAMPI-MODE.vst3` dentro una delle cartelle VST3:

   - `C:\Program Files\Common Files\VST3` (per tutti gli utenti)
   - `%LOCALAPPDATA%\Programs\Common\VST3` (solo per te, senza diritti di admin)

   Se la cartella `SPAMPI-MODE.vst3` esiste già, **sostituiscila** con quella nuova:
   non serve disinstallare nulla.

2. Riscansiona i plugin nel tuo DAW.

   - **Ableton Live**: `Preferenze > Plug-in > Riscansiona`. Se usi la cartella
     per-utente, aggiungi `%LOCALAPPDATA%\Programs\Common\VST3` in
     `Plug-in > "cartella VST3 personalizzata"`.
   - **Reaper**: `Opzioni > Preferenze > Plugin > VST: imposta la cartella VST3 e
     clicca "Aggiorna"`.
   - **Cubase / Studio One**: apri e chiudi la scansione dei plugin (o riavvia la DAW).

3. Cerca **SPAMPI-MODE** nella lista degli effetti e inseriscilo su una traccia.

## Primi passi

1. Metti il plugin su una traccia e suona una nota: la registrazione (di default la
   scimmia) parte alla velocità di quella nota.
2. Ogni registrazione è tagliata nel suo punto di picco: il colpo parte dal suono vero
   (l'urlo, l'abbaio, lo scarico), non dal silenzio prima. La linea `START` sulla forma
   d'onda è il punto da cui parte, e l'attacco della nota cade lì.
3. Con `TRIGGER` su `SHOT` (il default) senti **un colpo per nota**: la registrazione
   suona una volta e poi tace. Per farla ripartire lascia andare la nota. Se vuoi il
   nastro che gira di continuo, metti `LOOP`.
4. `FOLLOW` al 100% = segue la nota esatta. Abbassalo per un nastro più indipendente.
5. `ROOT` è la nota di riferimento della registrazione: quando cambi registrazione dal
   box `SAMPLE` si riallinea da sola alla nota misurata.
6. Se non sente niente, alza `LEVEL`, verifica `MIX` al 100% e `TRIGGER` (in `GATE`
   suona solo mentre c'è una nota).
7. `SENS` è la soglia: alzala se il rumore di fondo apre il gate, abbassala se le note
   morbide non vengono prese.

Non serve nessun routing MIDI: l'ingresso audio della traccia è già l'informazione che
gli serve. Se la traccia ha un altro effetto davanti, il plugin ascolta quello.

## Le registrazioni

Dodici suoni veri viaggiano dentro il plugin (tutti CC0, pubblico dominio).

`LOAD SAMPLE` importa un tuo file (`wav`, `aif`, `flac`, `ogg`, `mp3`): viene copiato
nella cartella qui sotto, misurato (così `ROOT` si mette in tono da solo) e
selezionato.

```
%APPDATA%\NULLCAM\SPAMPI-MODE\samples
```

Puoi anche copiare i file lì a mano e riaprire il plugin.

## Preset

I tuoi preset stanno in:

```
%APPDATA%\NULLCAM\SPAMPI-MODE\presets
```

Un file `.xml` per preset, leggibile: puoi copiarlo, salvarlo o mandarlo a un amico.
Il preset ricorda anche il percorso della registrazione usata.

Per cancellarli: il pulsante `DEL` nella barra in alto, oppure cancella il file.

## Problemi

**Non compare nella lista dei plugin.**
Controlla di aver copiato la cartella `SPAMPI-MODE.vst3` intera (con `Contents`
dentro) e riavvia/riscansiona la DAW.

**Non suona niente.**
Alza `LEVEL`, metti `MIX` al 100%, `FOLLOW` sopra 0%. Se `TRIGGER` è su `GATE`, deve
esserci una nota in ingresso. Attento: in Ableton il plugin va su una traccia con
ingresso audio attivo (se è una traccia MIDI vuota non ha nulla da ascoltare).

**Suona una volta e poi basta.**
È `SHOT`, ed è il comportamento giusto: un colpo per nota. Lascia andare la nota (o
cambiala) e riparte. Se vuoi che continui, scegli `LOOP`.

**Non sento l'inizio del suono.**
La registrazione è tagliata nel suo punto di picco, quindi il "prima" non viene suonato:
è voluto. La linea `START` sulla forma d'onda mostra dove comincia.

**La nota che legge è sbagliata o salta di ottava.**
Suona note pulite, una alla volta: il detector lavora su 2048 campioni. Abbassa
`SPREAD` (che sposta di proposito alcune note di un'ottava) e alza `GLIDE` per un
movimento più morbido. Se il suono è molto sporco, alza `SENS`.

**Sembra un chipmunk.**
`ROOT` è troppo basso rispetto alla registrazione: scegli la registrazione dal box
`SAMPLE` (che rimisura), oppure alza `ROOT` a mano. `INTERVAL` deve essere 0.

**Ho importato un sample e non lo vedo.**
`LOAD SAMPLE` lo copia in `%APPDATA%\NULLCAM\SPAMPI-MODE\samples` e lo aggiunge in
fondo alla lista `SAMPLE`. Se il file è più lungo di 60 secondi viene tagliato.

## Licenza e codice sorgente

SPAMPI-MODE è software libero: puoi usarlo, copiarlo e modificarlo secondo la
**GNU Affero General Public License v3** (o una versione successiva). Il plugin è
distribuito **senza garanzia** di alcun tipo.

Il codice sorgente completo corrispondente a questo binario è pubblico qui:

```
https://github.com/Philopium/NULLCAM-plugins
```

(licenza completa: `https://www.gnu.org/licenses/agpl-3.0.html`)
