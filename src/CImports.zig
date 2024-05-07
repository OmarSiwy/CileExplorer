const c = @cImport({
    @cInclude("GTK_Entry.h");
});

pub const run = c.run;
