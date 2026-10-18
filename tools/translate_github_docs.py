#!/usr/bin/env python3
"""English translation of Spanish prose in historical DCR Markdown documentation.

Keeps code blocks, literal commands, paths, URLs, inline code and repository
identifiers unchanged. Uses a local Spanish->English Marian model (no web API).
Translates documentation only; does not rewrite bilingual UI localization.
"""
import argparse
import collections
import json
import pathlib
import re
import subprocess

WORDS = re.compile(
    r"\b(?:de|del|el|la|los|las|una|unas|unos|para|por|con|sin|cuando|ahora|también|tambien|porque|"
    r"juego|juegos|versión|versiones|gráficos|sonido|archivos|corregido|corrección|problema|pasos|"
    r"nueva|nuevos|usuario|requiere|requieren|seleccionar|funciona|ejecutar|ejecuta|está|estaba|"
    r"puede|pueden|debe|deben|actualizado|añadir|agregar|guardado|prueba|pruebas|incluye|"
    r"mejoras|arregla|siempre|permite|consola|mandos|entrada|salida|cambios|pendiente|objetivo|"
    r"resumen|iniciar|terminar|usar|hacer|pantalla|ventana|carpeta|disco|botón|opción|opciones|"
    r"todas|todos|solo|además|donde|desde|tener|hasta|archivo|instrucciones|ejemplos|"
    r"del|que|qué|como|cómo|este|esta|estos|estas|primer|primera|después|luego|"
    r"revisar|revisión|agrega|carga|falta|notas|empezar|ejecutado|intento|intentos)\b", re.I
)
ACCENT = re.compile(r"[áéíóúñÁÉÍÓÚÑ¿¡]")
CODE = re.compile(r"(" + chr(96) + r"[^" + chr(96) + r"]+" + chr(96) +
    r"|\]\([^)]+\)|https?://[^\s)]+|%[A-Z0-9_]+%|\$[A-Z0-9_]+)", re.I)
MD_HEAD = re.compile(r"^(\s{0,3}(?:#{1,6}\s+|>\s*|[-*+]\s+|\d+\.\s+|\[[ xX]\]\s*))")
CODELINE = re.compile(r"^\s{4,}\S|^\s*(?:\$|PS [A-Z]:|[A-Z]:\\|set |if |for |echo |call |rem |::|#include|import |from |def |class )")
SUFFIXES = {".md", ".txt"}
EXCLUDE = {"docs/README.es.md"}
IGNORE_PREFIXES = ("build-v011/",)
# These short tokens can be ordinary game titles/identifiers and must not be translated.
NAME_TOKENS = ("SH-4", "ARM7", "AICA", "GD-ROM", "PVR", "VMU", "CDDA",
               "IP.BIN", "KYONB", "KYONEX", "DCR", "cpp38e", "DreamcastRecompiled")

def likely_spanish(line: str) -> bool:
    if len(line.strip()) < 5:
        return False
    words = WORDS.findall(line)
    return len(words) >= 2 or (len(words) >= 1 and bool(ACCENT.search(line))) or (
        len(line.strip()) < 90 and bool(ACCENT.search(line)) and len(line.split()) >= 2
    )

def split_line(line: str):
    m = MD_HEAD.match(line)
    prefix = m.group(0) if m else ""
    body = line[len(prefix):]
    # Avoid translating Markdown table and code markers.
    if not body.strip() or body.strip().startswith(("---","===","<!--","<","![")):
        return prefix, [(False, body)]
    # Keep inline code, URLs, Markdown link URLs and Windows env vars verbatim.
    result = []
    pos = 0
    for match in CODE.finditer(body):
        if pos < match.start():
            result.extend(split_format(body[pos:match.start()]))
        result.append((False, match.group(0)))
        pos = match.end()
    if pos < len(body):
        result.extend(split_format(body[pos:]))
    return prefix, result

def split_format(part: str):
    # Keep Markdown structural formatting, translate text between delimiters.
    pieces = re.split(r"(\*\*|__|(?<!\*)\*(?!\*)|\||~~|\[|\])", part)
    return [(token not in ("**","__","*","|","~~","[","]") and likely_spanish(token), token)
            for token in pieces if token]

def read_jobs(root: pathlib.Path):
    tracked = subprocess.check_output(["git","ls-files","-z"], cwd=root).decode("utf-8").split("\0")
    result = {}
    segments = collections.OrderedDict()
    for name in tracked:
        if not name or name in EXCLUDE or name.startswith(IGNORE_PREFIXES):
            continue
        p = root/name
        if p.suffix.lower() not in SUFFIXES or not p.is_file() or p.stat().st_size>150000:
            continue
        # Only project prose, not source-generated scripts or game media.
        if not (name.startswith("docs/") or name.startswith("game-portables/") or
                name in {"CHANGELOG.md","README.md","RELEASE_NOTES_V0.1.md",
                         "RELEASE_NOTES_V0.1.1.md","RELEASE_NOTES_V1.2.md"}):
            continue
        try: text = p.read_text(encoding="utf-8")
        except UnicodeDecodeError: continue
        lines = text.splitlines(keepends=True)
        fence = None
        parts = []
        for line in lines:
            core = line.rstrip("\r\n")
            tail = line[len(core):]
            stripped = core.strip()
            if stripped.startswith(("","")) and (stripped.startswith(chr(96)*3) or stripped.startswith("~~~")):
                delim = stripped[:3]
                if fence is None: fence = delim
                elif fence == delim: fence = None
                parts.append((None, core, tail))
                continue
            if fence is not None or CODELINE.match(core) or stripped.startswith(("<","!","|---")):
                parts.append((None, core, tail))
                continue
            prefix, pieces = split_line(core)
            for translated, token in pieces:
                if translated and token.strip():
                    # Keep terminal spacing and punctuation around model segments.
                    segments.setdefault(token, None)
            parts.append((prefix, pieces, tail))
        result[name] = parts
    return result, segments

def translate(model, tokenizer, text: list[str]):
    import torch
    torch.set_num_threads(2)
    lookup = {}
    batch_size = 18
    keys = list(text)
    for i in range(0, len(keys), batch_size):
        batch = keys[i:i+batch_size]
        # Limit truncation to long paragraphs. Do not invent missing ends.
        valid = [v for v in batch if len(v) < 950]
        for v in batch:
            if v not in valid: lookup[v] = v
        if not valid: continue
        # Leading/trailing indentation was kept as part of each token.
        clean = [v.strip() for v in valid]
        with torch.no_grad():
            values = tokenizer(clean, return_tensors="pt", truncation=True,
                               padding=True, max_length=400)
            outputs = model.generate(**values, num_beams=1, max_new_tokens=440)
        translated = tokenizer.batch_decode(outputs, skip_special_tokens=True)
        for original, candidate in zip(valid, translated):
            candidate = candidate.strip()
            if not candidate or len(candidate) > max(180, len(original)*3):
                lookup[original] = original
                continue
            lead = original[:len(original)-len(original.lstrip())]
            trail = original[len(original.rstrip()):]
            # None of these hardware interface names should change.
            if any(token in original and token not in candidate for token in NAME_TOKENS):
                lookup[original] = original
            else:
                lookup[original] = lead + candidate + trail
        if (i//batch_size) % 20 == 0:
            print(f"translated batches {i+len(batch)}/{len(keys)}", flush=True)
    return lookup

def apply(root: pathlib.Path, jobs, translations, report_dir: pathlib.Path):
    import tarfile
    changed=[]
    report_dir.mkdir(parents=True, exist_ok=True)
    with tarfile.open(report_dir/"translated-docs.tar.gz","w:gz") as tar:
        for name, lines in jobs.items():
            out=[]
            original=(root/name).read_text(encoding="utf-8")
            for prefix, body, tail in lines:
                if prefix is None:
                    out.append(body+tail)
                else:
                    out.append(prefix+"".join(translations.get(t,t) if translate else t
                        for translate,t in body)+tail)
            new = "".join(out)
            if new == original: continue
            # Do not damage Markdown references or inline code.
            if new.count(chr(96))!=original.count(chr(96)) or new.count("](")!=original.count("]("):
                print("SKIP integrity mismatch:",name)
                continue
            file=report_dir/name
            file.parent.mkdir(parents=True,exist_ok=True)
            file.write_text(new,encoding="utf-8")
            tar.add(file,arcname=name)
            changed.append((name,len(original),len(new)))
    (report_dir/"translated-docs-report.json").write_text(
        json.dumps({"changed":changed,"translations":len(translations)},indent=2),
        encoding="utf-8"
    )
    print("Translated documentation files:",len(changed),flush=True)
    for path, old, new in changed: print("CHANGED",path,old,new,flush=True)

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument("--out",default="english-translation-output")
    args=parser.parse_args()
    root=pathlib.Path(__file__).resolve().parents[2]
    jobs, segments=read_jobs(root)
    print("Detected Spanish prose spans:",len(segments),flush=True)
    from transformers import MarianTokenizer, MarianMTModel
    model_name="Helsinki-NLP/opus-mt-es-en"
    tokenizer=MarianTokenizer.from_pretrained(model_name)
    model=MarianMTModel.from_pretrained(model_name)
    lookup=translate(model,tokenizer,list(segments))
    apply(root,jobs,lookup,root/args.out)

if __name__=="__main__":
    main()
