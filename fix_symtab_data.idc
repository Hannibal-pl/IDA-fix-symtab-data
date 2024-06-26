#include <idc.idc>

//License: GPL-2.0+
//Author:  Konrad Rzepecki <hannibal@astral.lodz.pl>

extern rel_ea_start;
extern rel_ea_end;
extern new_seg_start;
extern new_seg_end;
extern last_sel;

static get_last_ea(void) {
    auto ea;
    auto nextea;
    auto sel;

    last_sel = 0;
    ea = get_first_seg();
    nextea = ea;
    while (nextea != BADADDR) {
        ea = nextea;

        //find last selector
        sel = get_segm_attr(ea, SEGATTR_SEL);
        if (sel > last_sel) {
            last_sel = sel;
        }

        nextea = get_next_seg(ea);
    }
    return get_segm_end(ea);
}

static get_rel_segment(void) {
    auto rel_ea;
    rel_ea = ask_addr(0x8056000, "Enter addres within relocable symbol segment (ELF .symtab)");
    rel_ea_start = get_segm_start(rel_ea);
    rel_ea_end = get_segm_end(rel_ea);
}

static create_new_seg(void) {
    auto i;
    auto new_seg_length;

    new_seg_start = get_last_ea() + 1;

    //find required size for new data segment
    for (i = rel_ea_start; i < rel_ea_end; i = i + 4) {
        new_seg_length = new_seg_length + get_wide_dword(i);
    }
    new_seg_end = new_seg_start + new_seg_length;

    set_selector(last_sel + 1, 0);
    add_segm_ex(new_seg_start, new_seg_end, last_sel + 1, 1, saRelByte, scPub, 0);
    set_segm_class(new_seg_start, "DATA");
    set_segm_attr(new_seg_start, SEGATTR_PERM, SEGPERM_WRITE | SEGPERM_READ);
    set_segm_name(new_seg_start, sprintf("%s.unpacked", get_segm_name(rel_ea_start)));
}

static patch_instr(xref, rel, neww, var_size) {
//this code is extremly stupid, because I don't know how to do it properly
//It search within instuction code addreses between "ref - var_size" and "ref + var_size".
//If found, save offset, then patch it to "new + offset".
    auto isize;
    auto i;
    auto wdword;
    auto wdword_lo;
    auto wdword_hi;
    auto offset;
    auto xtype;

    isize = get_item_size(xref) - 3;
    for (i = 0; i < isize; i++) {
        wdword = get_wide_dword(xref + i);
        wdword_lo = wdword - var_size;
        wdword_hi = wdword + var_size;

        offset = wdword - rel;

        if ((rel < wdword_lo) || (rel >  wdword_hi)) {
            continue;
        }

        patch_dword(xref + i, neww + offset);
        xtype = get_xref_type();
        del_dref(xref, rel);
        add_dref(xref, neww + offset, xtype);
        del_fixup(xref + i);
    }
}

static patch_code(rel, neww, var_size) {
    auto xref;

    xref = get_first_dref_to(rel);
    while (xref != BADADDR && (xref < new_seg_start)) {
        patch_instr(xref, rel, neww, var_size);
        xref = get_next_dref_to(rel, xref);
    }

    //delete additional xref to relocs - for some reason, when are more than one
    //to same values they are not deleted
    xref = get_first_dref_to(rel);
    while (xref != BADADDR && (xref < new_seg_start)) {
        del_dref(xref, rel);
        xref = get_next_dref_to(rel, xref);
    }
}

static move_vars(void) {
    auto rel;
    auto neww;
    auto var_size;
    auto var_name;

    rel = rel_ea_start;
    neww = new_seg_start;
    for(rel = rel_ea_start; rel < rel_ea_end; rel = rel + 4) {
        //for each entry in reloc segment create variable with desired size in new segment 
        var_size = get_wide_dword(rel);
        if (var_size == 1) {
            create_byte(neww);
        } else if (var_size == 2) {
            create_word(neww);
        } else if (var_size == 4) {
            create_dword(neww);
        } else {
            create_byte(neww);
            make_array(neww, var_size);
        }

        //copy variable name to new data with rename old one to *_rel
        var_name = get_name(rel);
        print(sprintf("Fixing var: %s", var_name));
        set_name(rel, sprintf("%s_rel", var_name));
        set_name(neww, var_name);

        //patch all code "xref to" old variable to point to new data
        patch_code(rel, neww, var_size);

        neww = neww + var_size;
    }
}

static main(void) {
    if (ask_yn(0, "This script heavy modify database and can damge it.\nIt's recomended to save database before you use it.\nDo you wish to continue?") != 1) {
        return;
    }

    get_rel_segment();
    create_new_seg();
    move_vars();
    print("Fix complete.");

    if (ask_yn(0, "Do you whish to realnalyze database?") == 1) {
        //this range could be narrowed, but this is good enouh.
        plan_and_wait(0, 0xFFFFFFFF);
    }
}
