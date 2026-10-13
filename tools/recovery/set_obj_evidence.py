#!/usr/bin/env python3
"""Inspect original STF set_obj assembly labels without requiring game ROMs."""
import argparse
import json
import re
from pathlib import Path

ASM=("rom_code1.s","rom_code2.s","rom_code3.s","rom_code4.s")
TARGETS={"set_obj","set_obj_tpd","set_obj_thd"}
LABEL=re.compile(r"^\s*([A-Za-z_$][\w.$]*):{1,2}(?!:)")
CALL=re.compile(r"\b(?:call|bal)\s+([A-Za-z_$][\w.$]*)\b",re.I)


def analyze(root,span=140):
    routines=[]
    calls={name:[] for name in TARGETS}
    for fn in ASM:
        lines=(root/"src/asm"/fn).read_text(encoding="utf-8",errors="replace").splitlines()
        defined=[]
        for i,line in enumerate(lines):
            m=LABEL.match(line)
            if m:
                defined.append((i,m.group(1)))
            for call in CALL.finditer(line.split("#",1)[0]):
                if call.group(1) in TARGETS:
                    calls[call.group(1)].append({"file":fn,"line":i+1,"text":line.strip()})
        for index,(start,name) in enumerate(defined):
            if name not in TARGETS:
                continue
            end=min(len(lines),start+span)
            later=defined[index+1] if index+1<len(defined) else None
            routines.append({
                "name":name,"file":fn,"line":start+1,
                "following_label":later[1] if later else None,
                "following_label_line":later[0]+1 if later else None,
                "excerpt":[{"line":i+1,"text":lines[i]} for i in range(start,end)]
            })
    return {"schema_version":1,"routines":routines,"calls":calls,
            "warning":"Following label is NOT a verified function end; source is STF Assembly, not a game ROM hardware capture."}


def markdown(r):
    out=["# STF set_obj disassembly inspection",
         "","These windows are original STF Assembly source, not proven function boundaries.",
         "","| Symbol | Definition | Direct calls |",
         "| --- | --- | ---: |"]
    for name in sorted(TARGETS):
        fn=next((x for x in r["routines"] if x["name"]==name),None)
        location=fn["file"]+":"+str(fn["line"]) if fn else "not found"
        out.append("| "+name+" | "+location+" | "+str(len(r["calls"][name]))+" |")
    for item in r["routines"]:
        out.extend(["","## "+item["name"]+" original source",
                    "Next label: "+str(item["following_label"])+" at line "+str(item["following_label_line"])+". It is not a function end.",
                    "","~~~asm"])
        out.extend(str(l["line"]).rjust(7)+": "+l["text"] for l in item["excerpt"])
        out.append("~~~")
    out.extend(["","Reference to compare: VF2 set_obj at 0x7C60 is oracle-proved for VF2 ONLY.",
                "STF and VF2 device rules must not be assumed identical.",""])
    return "\n".join(out)


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--root",type=Path,default=Path(__file__).resolve().parents[2])
    p.add_argument("--out-dir",type=Path,default=Path("build/set-obj"))
    p.add_argument("--span",type=int,default=140)
    a=p.parse_args()
    if a.span<10 or a.span>250:
        p.error("span must be 10..250")
    r=analyze(a.root.resolve(),a.span)
    a.out_dir.mkdir(parents=True,exist_ok=True)
    (a.out_dir/"set_obj_evidence.json").write_text(json.dumps(r,indent=2)+"\n")
    m=markdown(r)
    (a.out_dir/"SET_OBJ_ASSEMBLY.md").write_text(m)
    print(m)


if __name__=="__main__":
    main()
