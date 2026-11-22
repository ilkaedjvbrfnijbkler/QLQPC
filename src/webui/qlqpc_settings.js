// ES5 settings overlay for Quake Live WebUI (Awesomium Chromium 18 / V8).

(function (global)
{
    "use strict";

    // Keys must remain strictly lowercase to match GameSettings.renderCvar & cvardb lookups.
    var QLQPC_CVAR_DEFS = [
        {
            name: "cl_cmdrate",
            title: "Outbound Rate",
            min: "0",
            max: "250",
            step: "",
            type: "select",
            defaultFallback: "250",
            options: [
                { value: "250", label: "250Hz" },
                { value: "144", label: "144Hz" },
                { value: "0", label: "Undecoupled" }
            ]
        },
        {
            name: "cg_drawaccuracy",
            title: "Show Client LG Accuracy",
            min: "0",
            max: "1",
            step: "",
            type: "bool",
            defaultFallback: "0"
        },
        {
            name: "cg_drawotherweapons",
            title: "Other Players' Weapons",
            min: "0",
            max: "1",
            step: "",
            type: "bool",
            defaultFallback: "1"
        },
        {
            name: "cg_teamlightningstyle",
            title: "Team Lightning Style",
            min: "0",
            max: "5",
            step: "",
            type: "select",
            defaultFallback: "1",
            options: [
                { value: "0", label: "Beamless" },
                { value: "1", label: "Default" },
                { value: "2", label: "QuakeWorld" },
                { value: "3", label: "Team Arena" },
                { value: "4", label: "Thin Shaft" },
                { value: "5", label: "Wide Beam" }
            ]
        },
        {
            name: "cg_teamlightningopacity",
            title: "Team Lightning Opacity",
            min: "0.00",
            max: "1.00",
            step: "0.05",
            type: "range",
            defaultFallback: "1.00"
        },
        {
            name: "cg_enemylightningstyle",
            title: "Enemy Lightning Style",
            min: "0",
            max: "5",
            step: "",
            type: "select",
            defaultFallback: "1",
            options: [
                { value: "0", label: "Beamless" },
                { value: "1", label: "Default" },
                { value: "2", label: "QuakeWorld" },
                { value: "3", label: "Team Arena" },
                { value: "4", label: "Thin Shaft" },
                { value: "5", label: "Wide Beam" }
            ]
        },
        {
            name: "cg_enemylightningopacity",
            title: "Enemy Lightning Opacity",
            min: "0.00",
            max: "1.00",
            step: "0.05",
            type: "range",
            defaultFallback: "1.00"
        },
        {
            name: "r_vulkan",
            title: "Graphics API",
            min: "0",
            max: "1",
            step: "",
            type: "select",
            defaultFallback: "0",
            options: [
                { value: "0", label: "OpenGL" },
                { value: "1", label: "Vulkan" }
            ]
        },
        {
            name: "r_fog",
            title: "Atmospheric Fog",
            min: "0",
            max: "1",
            step: "",
            type: "bool",
            defaultFallback: "0"
        },
        {
            name: "r_fogcolor",
            title: "Fog Color",
            min: "",
            max: "",
            step: "",
            type: "color"
        },
        {
            name: "r_fogdistance",
            title: "Fog Distance",
            min: "10",
            max: "10000",
            step: "10",
            type: "range",
            defaultFallback: "768"
        },
        {
            name: "r_fogintensity",
            title: "Fog Density",
            min: "0.05",
            max: "10.00",
            step: "0.05",
            type: "range",
            defaultFallback: "1.00"
        },
        {
            name: "r_fogoverridenative",
            title: "Override Map Fog",
            min: "0",
            max: "1",
            step: "",
            type: "bool",
            defaultFallback: "1"
        },
        {
            name: "cg_forcewhitenames",
            title: "Force White Names",
            min: "0",
            max: "1",
            step: "",
            type: "bool",
            defaultFallback: "0"
        }
    ];

    var QLQPC_CVAR_INDEX = {};
    for (var d = 0; d < QLQPC_CVAR_DEFS.length; d++)
    {
        QLQPC_CVAR_INDEX[QLQPC_CVAR_DEFS[d].name] = QLQPC_CVAR_DEFS[d];
    }

    function isNil(val)
    {
        return val === null || val === undefined;
    }

    function cloneElementWithChildren(React, element, children)
    {
        return React.cloneElement.apply(React, [element, null].concat(children));
    }

    function childrenToArray(React, childrenProp)
    {
        if (isNil(childrenProp))
        {
            return [];
        }
        var arr = [];
        React.Children.forEach(childrenProp, function (child)
        {
            arr.push(child);
        });
        return arr;
    }

    function transformTree(React, node, predicate, transform, dropNull)
    {
        if (!node || typeof node !== "object" || !node.props)
        {
            return node;
        }
        if (predicate(node))
        {
            return transform(node);
        }
        if (isNil(node.props.children))
        {
            return node;
        }
        var rawChildren = childrenToArray(React, node.props.children);
        var changed = false;
        var nextChildren = [];
        for (var i = 0; i < rawChildren.length; i++)
        {
            var orig = rawChildren[i];
            var updated = transformTree(React, orig, predicate, transform, dropNull);
            if (updated !== orig)
            {
                changed = true;
            }
            if (!dropNull || updated !== null)
            {
                nextChildren.push(updated);
            }
        }
        return changed ? cloneElementWithChildren(React, node, nextChildren) : node;
    }

    function ensureCustomSelectOption(cvaroptions, cvarName, rawValue)
    {
        var def = QLQPC_CVAR_INDEX[cvarName];
        if (!def || !def.options)
        {
            return;
        }
        var cloned = def.options.slice();
        var val = rawValue;
        if (isNil(val) || val === "")
        {
            val = def.defaultFallback || "";
        }
        else
        {
            val = String(val);
        }
        if (val !== "")
        {
            var found = false;
            for (var i = 0; i < cloned.length; i++)
            {
                if (cloned[i].value === val)
                {
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                cloned.unshift({ value: val, label: val + " (Custom)" });
            }
        }
        cvaroptions[cvarName] = cloned;
    }

    function renderSafeCvar(ctx, cvaroptions, cvarName)
    {
        var def = QLQPC_CVAR_INDEX[cvarName];
        var cvarsState = (ctx.state && ctx.state.cvars) ? ctx.state.cvars : null;
        if (!cvarsState)
        {
            return null;
        }
        ensureCustomSelectOption(cvaroptions, cvarName, cvarsState[cvarName]);

        // Guard Cvar type="color" (Utils.hex2rgb requires an 8+ char hex string).
        var hadKey = Object.prototype.hasOwnProperty.call(cvarsState, cvarName);
        var origVal = cvarsState[cvarName];
        var injectedTemp = false;
        if (def && def.type === "color")
        {
            if (!origVal || typeof origVal !== "string" || origVal.length < 8)
            {
                cvarsState[cvarName] = "0xffffffff";
                injectedTemp = true;
            }
        }
        else if (!hadKey && def && !isNil(def.defaultFallback))
        {
            cvarsState[cvarName] = def.defaultFallback;
            injectedTemp = true;
        }

        var el = ctx.renderCvar(cvarName);

        if (injectedTemp)
        {
            if (hadKey)
            {
                cvarsState[cvarName] = origVal;
            }
            else
            {
                delete cvarsState[cvarName];
            }
        }
        return el;
    }

    function stripFilteredCvarNodes(React, node, filterMap)
    {
        return transformTree(
            React,
            node,
            function (cur)
            {
                return typeof cur.props.name === "string" && Boolean(filterMap[cur.props.name.toLowerCase()]);
            },
            function ()
            {
                return null;
            },
            true
        );
    }

    function insertIntoColumn(colKids, extras, afterName)
    {
        if (!extras || extras.length === 0)
        {
            return false;
        }
        var valid = [];
        for (var i = 0; i < extras.length; i++)
        {
            if (extras[i])
            {
                valid.push(extras[i]);
            }
        }
        if (valid.length === 0)
        {
            return false;
        }
        var insertAt = colKids.length;
        if (afterName)
        {
            var anchor = String(afterName).toLowerCase();
            var foundAnchor = false;
            for (var k = 0; k < colKids.length; k++)
            {
                var child = colKids[k];
                if (child && child.props && typeof child.props.name === "string" &&
                    child.props.name.toLowerCase() === anchor)
                {
                    insertAt = k + 1;
                    foundAnchor = true;
                    break;
                }
            }
            if (!foundAnchor)
            {
                throw new Error("qlqpc: missing settings anchor " + afterName);
            }
        }
        Array.prototype.splice.apply(colKids, [insertAt, 0].concat(valid));
        return true;
    }

    function appendToRowColumns(React, root, leftExtras, rightExtras, filterNames, insertAfter)
    {
        var filterMap = null;
        if (filterNames && filterNames.length > 0)
        {
            filterMap = {};
            for (var f = 0; f < filterNames.length; f++)
            {
                filterMap[String(filterNames[f]).toLowerCase()] = true;
            }
        }
        var patched = false;
        return transformTree(
            React,
            root,
            function (node)
            {
                if (patched)
                {
                    return false;
                }
                return (
                    node.type === "div" &&
                    node.props &&
                    typeof node.props.className === "string" &&
                    node.props.className.indexOf("row") !== -1
                );
            },
            function (rowNode)
            {
                var cols = childrenToArray(React, rowNode.props.children);
                var halfIdx = 0;
                var nextCols = [];
                for (var i = 0; i < cols.length; i++)
                {
                    var col = cols[i];
                    if (
                        col &&
                        col.props &&
                        typeof col.props.className === "string" &&
                        col.props.className.indexOf("one-half") !== -1
                    )
                    {
                        if (filterMap)
                        {
                            var filteredCol = stripFilteredCvarNodes(React, col, filterMap);
                            if (!isNil(filteredCol))
                            {
                                col = filteredCol;
                            }
                        }
                        var colKids = childrenToArray(React, col.props.children);
                        var extras = halfIdx === 0 ? leftExtras : halfIdx === 1 ? rightExtras : null;
                        var afterName = insertAfter ? (halfIdx === 0 ? insertAfter.left : halfIdx === 1 ? insertAfter.right : null) : null;
                        halfIdx++;
                        if (insertIntoColumn(colKids, extras, afterName))
                        {
                            col = cloneElementWithChildren(React, col, colKids);
                        }
                    }
                    nextCols.push(col);
                }
                if (halfIdx > 0)
                {
                    patched = true;
                    return cloneElementWithChildren(React, rowNode, nextCols);
                }
                return rowNode;
            }
        );
    }

    function registerCvarMetadata(cvardb, cvaroptions)
    {
        for (var i = 0; i < QLQPC_CVAR_DEFS.length; i++)
        {
            var def = QLQPC_CVAR_DEFS[i];
            cvardb[def.name] = {
                name: def.name,
                title: def.title,
                min: def.min,
                max: def.max,
                step: def.step,
                type: def.type
            };
            if (def.options)
            {
                cvaroptions[def.name] = def.options.slice();
            }
        }
    }

    function wrapSection(proto, sectionName, buildFn)
    {
        var orig = proto.sections[sectionName];
        if (typeof orig !== "function" || orig.__qlqpcWrapped)
        {
            return;
        }
        var wrapped = function ()
        {
            var root = orig.apply(this, arguments);
            return buildFn(this, root);
        };
        wrapped.__qlqpcWrapped = true;
        proto.sections[sectionName] = wrapped;
    }

    var installed = false;

    function installQlqpcWebUiExtension()
    {
        if (installed || typeof global.req !== "function")
        {
            return installed;
        }

        var React = global.req(220);
        var cvardb = global.req(229);
        var cvaroptions = global.req(230);
        var GameSettings = global.req(235);

        if (!React || !cvardb || !cvardb.cg_fov ||
            !cvaroptions || !cvaroptions.cg_drawgun || !GameSettings ||
            !GameSettings.prototype || !GameSettings.prototype.sections)
        {
            return false;
        }

        installed = true;
        registerCvarMetadata(cvardb, cvaroptions);

        var proto = GameSettings.prototype;

        // Game tab: Outbound Rate & Force White Names
        wrapSection(proto, "game", function (ctx, root)
        {
            return appendToRowColumns(
                React,
                root,
                [
                    renderSafeCvar(ctx, cvaroptions, "cl_cmdrate"),
                    renderSafeCvar(ctx, cvaroptions, "cg_forcewhitenames")
                ],
                []
            );
        });

        // HUD tab: Show Client LG Accuracy
        wrapSection(proto, "hud", function (ctx, root)
        {
            return appendToRowColumns(
                React,
                root,
                [renderSafeCvar(ctx, cvaroptions, "cg_drawaccuracy")],
                []
            );
        });

        // Team tab: Team & Enemy Lightning Styles + Opacity
        wrapSection(proto, "team", function (ctx, root)
        {
            return appendToRowColumns(
                React,
                root,
                [
                    renderSafeCvar(ctx, cvaroptions, "cg_teamlightningstyle"),
                    renderSafeCvar(ctx, cvaroptions, "cg_teamlightningopacity")
                ],
                [
                    renderSafeCvar(ctx, cvaroptions, "cg_enemylightningstyle"),
                    renderSafeCvar(ctx, cvaroptions, "cg_enemylightningopacity")
                ]
            );
        });

        // Weapons tab: Weapon visibility (remove stock cg_lightningstyle; beam controls live under Team)
        wrapSection(proto, "weapons", function (ctx, root)
        {
            return appendToRowColumns(
                React,
                root,
                [renderSafeCvar(ctx, cvaroptions, "cg_drawotherweapons")],
                [],
                ["cg_lightningstyle"]
            );
        });

        // Video tab: Graphics API below Vertical Sync; Atmospheric Fog in right column
        wrapSection(proto, "video", function (ctx, root)
        {
            return appendToRowColumns(
                React,
                root,
                [renderSafeCvar(ctx, cvaroptions, "r_vulkan")],
                [
                    renderSafeCvar(ctx, cvaroptions, "r_fog"),
                    renderSafeCvar(ctx, cvaroptions, "r_fogcolor"),
                    renderSafeCvar(ctx, cvaroptions, "r_fogdistance"),
                    renderSafeCvar(ctx, cvaroptions, "r_fogintensity"),
                    renderSafeCvar(ctx, cvaroptions, "r_fogoverridenative")
                ],
                null,
                { left: "r_swapinterval" }
            );
        });

        return true;
    }

    var prevMainHook = global.main_hook_v2;
    global.main_hook_v2 = function ()
    {
        if (typeof prevMainHook === "function")
        {
            prevMainHook.apply(this, arguments);
        }
        installQlqpcWebUiExtension();
    };

    // If bundle.js already assigned window.req synchronously, install immediately.
    installQlqpcWebUiExtension();
}(window));
