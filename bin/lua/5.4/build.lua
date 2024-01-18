

local format = string.format;

local function build_file(file, outdir)
  local outfile = outdir .. "/" .. file;
  local ok = os.compile(file, outfile);
  if ok then
    print(format("%s build OK", file));
  else
    error(format("%s build failed", file));
  end  
end

local function build_dir(dir, outdir)
  local path = os.opendir(dir);
  if not path then
    return;
  end
  os.mkdir(outdir);
  os.mkdir(format("%s/%s", outdir, dir));
  for name, isdir in pairs(path) do
    local file = format("%s/%s", dir, name);
    if isdir then
      if name ~= outdir then
        build_dir(file, outdir);
      end
    else
      build_file(file, outdir);
    end
  end
end

function main(outdir)
  build_dir("lua", outdir or "~build");
end
