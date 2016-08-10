local ColorerGUID = "D2F36B62-A470-418D-83A3-ED7A3710E5B5";

local function ColorerProcess()
  local KeysArray = {
    ["Alt'"] = "6",    -- find errors
    ["RAlt'"] = "6",    -- find errors
    ["Alt;"] = "5",    -- list functions
    ["RAlt;"] = "5",    -- list functions
    AltK     = "7",    -- select region
    RAltK     = "7",    -- select region
    F5       = "5",    -- list functions
    AltL     = "1",    -- list types
    RAltL     = "1",    -- list types
    AltO     = "8",    -- find function
    RAltO     = "8",    -- find function
    AltP     = "4",    -- select pair
    RAltP     = "4",    -- select pair
    AltR     = "R",    -- reload colorer
    RAltR     = "R",    -- reload colorer
    ["Alt["] = "2",    -- match pair
    ["RAlt["] = "2",    -- match pair
    ["Alt]"] = "3",    -- select block
    ["RAlt]"] = "3",    -- select block
    CtrlShiftC = "C Tab",     -- cross on/off
    RCtrlShiftC = "C Tab"     -- cross on/off
  };
  local akey = mf.akey(2);
  local key = KeysArray[mf.akey(2)];
  Plugin.Menu(ColorerGUID) ;
  if Object.CheckHotkey(key) ~= 0 then
    Keys(key);
    if  akey == "CtrlShiftC" or akey == "RCtrlShiftC" then
      State=Dlg.GetValue(3, 0);
      if State==0 then Keys("Add") else Keys("Subtract") end
      Keys("Enter")
    end
  else
    Keys("Esc")
  end
end

Macro {
  area="Editor";
  description="Colorer: Find Errors";
  key="/.Alt'/";
  condition = function()
    return Plugin.Exist(ColorerGUID)
  end;
  action=ColorerProcess;
}

Macro {
  area="Editor";
  description="Colorer: List Functions";
  key="/.Alt;/";
  condition = function()
    return Plugin.Exist(ColorerGUID)
  end;
  action=ColorerProcess;
}

Macro {
  area="Editor";
  description="Colorer: List Functions";
  key="F5";
  condition = function()
    return Plugin.Exist(ColorerGUID)
  end;
  action=ColorerProcess;
}

Macro {
  area="Editor";
  description="Colorer: Select Region";
  key="/.AltK/";
  condition = function()
    return Plugin.Exist(ColorerGUID)
  end;
  action=ColorerProcess;
}

Macro {
  area="Editor";
  description="Colorer: List Types";
  key="/.AltL/";
  condition = function()
    return Plugin.Exist(ColorerGUID)
  end;
  action=ColorerProcess;
}

Macro {
  area="Editor";
  description="Colorer: Find Function";
  key="/.AltO/";
  condition = function()
    return Plugin.Exist(ColorerGUID)
  end;
  action=ColorerProcess;
}

Macro {
  area="Editor";
  description="Colorer: Select Pair";
  key="/.AltP/";
  condition = function()
    return Plugin.Exist(ColorerGUID)
  end;
  action=ColorerProcess;
}

Macro {
  area="Editor";
  description="Colorer: Reload Colorer";
  key="/.AltR/";
  condition = function()
    return Plugin.Exist(ColorerGUID)
  end;
  action=ColorerProcess;
}

Macro {
  area="Editor";
  description="Colorer: Match Pair";
  key="/.Alt\\[/";
  condition = function()
    return Plugin.Exist(ColorerGUID)
  end;
  action=ColorerProcess;
}

Macro {
  area="Editor";
  description="Colorer: Select Block";
  key="/.Alt]/";
  condition = function()
    return Plugin.Exist(ColorerGUID)
  end;
  action=ColorerProcess;
}

Macro {
  area="Editor";
  description="Colorer: Cross on/off";
  key="/.CtrlShiftC/";
  condition = function()
    return Plugin.Exist(ColorerGUID)
  end;
  action=ColorerProcess;
}
