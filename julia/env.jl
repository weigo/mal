module env

export Env, set, find, get

type Env
    outer::Any
    data::Dict{Symbol,Any}
end

function Env()
    Env(nothing, Dict())
end

function Env(outer)
    Env(outer, Dict())
end

function set(env::Env, k::Symbol, v)
    env.data[k] = v
end

function find(env::Env, k::Symbol)
    if haskey(env.data, k)
        env
    elseif env.outer != nothing
        find(env.outer, k)
    else
        nothing
    end
end

function get(env::Env, k::Symbol)
    e = find(env, k)
    if e != nothing
        e.data[k]
    else
        error("'$(string(k))' not found")
    end
end

end
