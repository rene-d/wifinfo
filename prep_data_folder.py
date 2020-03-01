#!/usr/bin/env python3
# module téléinformation client
# rene-d 2020

# requires: https://github.com/kangax/html-minifier

import shutil
from pathlib import Path
import subprocess
import os
import re
import tempfile


def get_minifier(suffix):

    if suffix in set([".htm", ".html"]):
        return (
            "html-minifier --collapse-whitespace --remove-comments --remove-optional-tags \
--remove-redundant-attributes --remove-script-type-attributes --remove-tag-whitespace \
--use-short-doctype --minify-css true --minify-js true {S} | gzip > {D}",
            ".gz",
        )

    if suffix in set([".js", ".css", ".ttf"]):
        return "gzip -9 < {S} > {D}", ".gz"

    return None, None


def trace(*args):
    print("\033[96m", end="")
    print(*args)
    print("\033[0m", end="")


def get_version():
    """
    Read the git version
    """

    if Path(".git").is_dir():
        cmd = ["git", "describe", "--long", "--always", "--tags", "--all"]
        wifinfo_version = subprocess.check_output(cmd).decode().strip()
        wifinfo_version = wifinfo_version[wifinfo_version.index("/") + 1 :]
    else:
        wifinfo_version = os.getenv("WIFINFO_VERSION", "develop")

    trace(f"  WIFINFO_VERSION = {wifinfo_version}")
    return wifinfo_version


def make_restrict(data_src_dir: Path, src_file: Path, data_dir: Path) -> None:

    html = src_file.read_text()
    if "<!--MAKE_RESTRICT-->" not in html:
        return

    html = re.sub(r"<!--RESTRICT_ON-->(.*?)<!--RESTRICT_OFF-->", "", html, flags=re.DOTALL)

    t = tempfile.NamedTemporaryFile("wt+")
    t.write(html)
    t.flush()

    cmd, suffix = get_minifier(src_file.suffix)

    # no really clear... to be simplified
    dest_file = data_dir / src_file.relative_to(data_src_dir)
    suffix = ".restrict" + "".join(dest_file.suffixes) + suffix
    dest_file = dest_file.with_suffix("")
    dest_file = dest_file.with_name(dest_file.name + suffix)

    cmd = cmd.format(S=t.name, D=dest_file.as_posix())
    subprocess.check_call(cmd, shell=True)
    trace(f"  restricted: {src_file.relative_to(data_src_dir)} → {dest_file.relative_to(data_dir)}")


def prepare_www_files(source, target, env) -> None:
    """"
     WARNING -  this script will DELETE your 'data' dir and recreate an empty one to copy/gzip files from 'data_src'
                so make sure to edit your files in 'data_src' folder as changes madt to files in 'data' woll be LOST

                If 'data_src' dir doesn't exist, and 'data' dir is found, the script will autimatically
                rename 'data' to 'data_src
    """

    trace("[PREPARE DATA FILES]")

    data_dir = Path(env.get("PROJECTDATA_DIR"))
    project_dir = Path(env.get("PROJECT_DIR"))

    trace(f"  PROJECTDATA_DIR = {data_dir}")
    trace(f"  PROJECT_DIR     = {project_dir}")

    data_src_dir = project_dir / "data_src"

    if data_dir.is_dir() and not data_src_dir.is_dir():
        trace(f'  "data" dir exists, "data_src" not found.')
        trace(f'  renaming "{data_dir}" to "{data_src_dir}"')
        data_dir.rename(data_src_dir)

    # if data_dir.exists():
    #     trace(f"  Deleting data dir: {data_dir}")
    #     shutil.rmtree(data_dir)
    # trace(f"  Re-creating empty data dir: {data_dir}")

    data_dir.mkdir(exist_ok=True, parents=True)

    # clean destination directory
    entries_to_remove = []
    for dest_file in data_dir.rglob("*"):
        src_file = data_src_dir / dest_file.relative_to(data_dir)
        if not src_file.exists():
            if src_file.suffix == ".gz":
                if src_file.with_suffix("").exists():
                    continue
            if ".restrict" in src_file.suffixes:
                continue
            entries_to_remove.append(dest_file)

    for i in entries_to_remove:
        trace(f"  remove: {i}")
        if i.is_dir():
            shutil.rmtree(i)
        elif i.exists():
            i.unlink()

    for src_file in data_src_dir.rglob("*"):
        if not src_file.is_file():
            continue

        # do we have a minifier for the given suffix ?
        cmd, suffix = get_minifier(src_file.suffix)
        dest_file = data_dir / src_file.relative_to(data_src_dir)

        # yes: modify the destination file suffix
        if cmd:
            dest_file = dest_file.with_name(dest_file.name + suffix)

        # file has been modified ?
        if dest_file.is_file():
            if dest_file.stat().st_mtime_ns >= src_file.stat().st_mtime_ns:
                trace(f"  unmodified: {dest_file.relative_to(data_dir)}")
                continue

        dest_file.parent.mkdir(exist_ok=True, parents=True)
        if cmd:
            cmd = cmd.format(S=src_file.as_posix(), D=dest_file.as_posix())
            subprocess.check_call(cmd, shell=True)
            trace(f"  minified:   {src_file.relative_to(data_src_dir)} → {dest_file.relative_to(data_dir)}")

            if src_file.suffix == ".html":
                make_restrict(data_src_dir, src_file, data_dir)

        else:
            shutil.copy2(src_file, dest_file)
            trace(f"  copied:     {dest_file.relative_to(data_dir)}")

    (data_dir / "version").write_text(get_version())

    trace("[/PREPARE DATA FILES]\033[0m")


if __name__ != "__main__":
    Import("env", "projenv")
    env.AddPreAction("$BUILD_DIR/spiffs.bin", prepare_www_files)
    projenv.Append(CPPDEFINES=[("WIFINFO_VERSION", '\\"' + get_version() + '\\"')])
else:
    prepare_www_files(None, None, {"PROJECTDATA_DIR": "data", "PROJECT_DIR": "."})
