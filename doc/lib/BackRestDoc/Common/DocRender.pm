####################################################################################################################################
# DOC RENDER MODULE
####################################################################################################################################
package BackRestDoc::Common::DocRender;

use strict;
use warnings FATAL => qw(all);
use Carp qw(confess);

use Exporter qw(import);
    our @EXPORT = qw();
use JSON::PP;
use Storable qw(dclone);

use pgBackRest::Common::Log;
use pgBackRest::Common::String;

use BackRestDoc::Common::DocManifest;

####################################################################################################################################
# XML tag/param constants
####################################################################################################################################
use constant XML_SECTION_PARAM_ANCHOR                               => 'anchor';
    push @EXPORT, qw(XML_SECTION_PARAM_ANCHOR);
use constant XML_SECTION_PARAM_ANCHOR_VALUE_NOINHERIT               => 'no-inherit';
    push @EXPORT, qw(XML_SECTION_PARAM_ANCHOR_VALUE_NOINHERIT);

####################################################################################################################################
# Render tags for various output types
####################################################################################################################################
my $oRenderTag =
{
    'markdown' =>
    {
        'quote' => ['"', '"'],
        'b' => ['**', '**'],
        'i' => ['_', '_'],
        # 'bi' => ['_**', '**_'],
        'ul' => ["\n", ""],
        'ol' => ["\n", "\n"],
        'li' => ['- ', "\n"],
        'id' => ['`', '`'],
        'file' => ['`', '`'],
        'path' => ['`', '`'],
        'cmd' => ['`', '`'],
        'param' => ['`', '`'],
        'setting' => ['`', '`'],
        'pg-setting' => ['`', '`'],
        'code' => ['`', '`'],
        # 'code-block' => ['```', '```'],
        # 'exe' => [undef, ''],
        'backrest' => [undef, ''],
        'proper' => ['', ''],
        'postgres' => ['PostgreSQL', '']
    },

    'text' =>
    {
        'quote' => ['"', '"'],
        'b' => ['', ''],
        'i' => ['', ''],
        # 'bi' => ['', ''],
        'ul' => ["\n", "\n"],
        'ol' => ["\n", "\n"],
        'li' => ['* ', "\n"],
        'id' => ['', ''],
        'host' => ['', ''],
        'file' => ['', ''],
        'path' => ['', ''],
        'cmd' => ['', ''],
        'br-option' => ['', ''],
        'pg-setting' => ['', ''],
        'param' => ['', ''],
        'setting' => ['', ''],
        'code' => ['', ''],
        'code-block' => ['', ''],
        'exe' => [undef, ''],
        'backrest' => [undef, ''],
        'proper' => ['', ''],
        'postgres' => ['PostgreSQL', '']
    },

    'latex' =>
    {
        'quote' => ['``', '"'],
        'b' => ['\textbf{', '}'],
        'i' => ['\textit{', '}'],
        # 'bi' => ['', ''],
        # 'ul' => ["\n", "\n"],
        # 'ol' => ["\n", "\n"],
        # 'li' => ['* ', "\n"],
        'id' => ['\textnormal{\texttt{', '}}'],
        'host' => ['\textnormal{\textbf{', '}}'],
        'file' => ['\textnormal{\texttt{', '}}'],
        'path' => ['\textnormal{\texttt{', '}}'],
        'cmd' => ['\textnormal{\texttt{', "}}"],
        'user' => ['\textnormal{\texttt{', '}}'],
        'br-option' => ['', ''],
        # 'param' => ['\texttt{', '}'],
        # 'setting' => ['\texttt{', '}'],
        'br-option' => ['\textnormal{\texttt{', '}}'],
        'br-setting' => ['\textnormal{\texttt{', '}}'],
        'pg-option' => ['\textnormal{\texttt{', '}}'],
        'pg-setting' => ['\textnormal{\texttt{', '}}'],
        'code' => ['\textnormal{\texttt{', '}}'],
        # 'code' => ['\texttt{', '}'],
        # 'code-block' => ['', ''],
        # 'exe' => [undef, ''],
        'backrest' => [undef, ''],
        'proper' => ['\textnormal{\texttt{', '}}'],
        'postgres' => ['PostgreSQL', '']
    },

    'html' =>
    {
        'quote' => ['<q>', '</q>'],
        'b' => ['<b>', '</b>'],
        'i' => ['<i>', '</i>'],
        # 'bi' => ['<i><b>', '</b></i>'],
        'ul' => ['<ul>', '</ul>'],
        'ol' => ['<ol>', '</ol>'],
        'li' => ['<li>', '</li>'],
        'id' => ['<span class="id">', '</span>'],
        'host' => ['<span class="host">', '</span>'],
        'file' => ['<span class="file">', '</span>'],
        'path' => ['<span class="path">', '</span>'],
        'cmd' => ['<span class="cmd">', '</span>'],
        'user' => ['<span class="user">', '</span>'],
        'br-option' => ['<span class="br-option">', '</span>'],
        'br-setting' => ['<span class="br-setting">', '</span>'],
        'pg-option' => ['<span class="pg-option">', '</span>'],
        'pg-setting' => ['<span class="pg-setting">', '</span>'],
        'code' => ['<span class="id">', '</span>'],
        'code-block' => ['<code-block>', '</code-block>'],
        'exe' => [undef, ''],
        'setting' => ['<span class="br-setting">', '</span>'], # ??? This will need to be fixed
        'backrest' => [undef, ''],
        'proper' => ['<span class="host">', '</span>'],
        'postgres' => ['<span class="postgres">PostgreSQL</span>', '']
    }
};

####################################################################################################################################
# CONSTRUCTOR
####################################################################################################################################
sub new
{
    my $class = shift;       # Class name

    # Create the class hash
    my $self = {};
    bless $self, $class;

    # Assign function parameters, defaults, and log debug info
    (
        my $strOperation,
        $self->{strType},
        $self->{oManifest},
        $self->{strRenderOutKey},
    ) =
        logDebugParam
        (
            __PACKAGE__ . '->new', \@_,
            {name => 'strType'},
            {name => 'oManifest'},
            {name => 'strRenderOutKey', required => false}
        );

    # Create JSON object
    $self->{oJSON} = JSON::PP->new()->allow_nonref();

    # Initialize project tags
    $$oRenderTag{markdown}{backrest}[0] = "{[project]}";
    $$oRenderTag{markdown}{exe}[0] = "{[project-exe]}";

    $$oRenderTag{text}{backrest}[0] = "{[project]}";
    $$oRenderTag{text}{exe}[0] = "{[project-exe]}";

    $$oRenderTag{latex}{backrest}[0] = "{[project]}";
    $$oRenderTag{latex}{exe}[0] = "\\textnormal\{\\texttt\{[project-exe]}}\}\}";

    $$oRenderTag{html}{backrest}[0] = "<span class=\"backrest\">{[project]}</span>";
    $$oRenderTag{html}{exe}[0] = "<span class=\"file\">{[project-exe]}</span>";

    if (defined($self->{strRenderOutKey}))
    {
        # Copy page data to self
        my $oRenderOut =
            $self->{oManifest}->renderOutGet($self->{strType} eq 'latex' ? 'pdf' : $self->{strType}, $self->{strRenderOutKey});

        # If these are the backrest docs then load the reference
        if ($self->{oManifest}->isBackRest())
        {
            $self->{oReference} =
                new BackRestDoc::Common::DocConfig(${$self->{oManifest}->sourceGet('reference')}{doc}, $self);
        }

        if (defined($$oRenderOut{source}) && $$oRenderOut{source} eq 'reference' && $self->{oManifest}->isBackRest())
        {
            if ($self->{strRenderOutKey} eq 'configuration')
            {
                $self->{oDoc} = $self->{oReference}->helpConfigDocGet();
            }
            elsif ($self->{strRenderOutKey} eq 'command')
            {
                $self->{oDoc} = $self->{oReference}->helpCommandDocGet();
            }
            else
            {
                confess &log(ERROR, "cannot render $self->{strRenderOutKey} from source $$oRenderOut{source}");
            }
        }
        elsif (defined($$oRenderOut{source}) && $$oRenderOut{source} eq 'release' && $self->{oManifest}->isBackRest())
        {
            require BackRestDoc::Custom::DocCustomRelease;
            BackRestDoc::Custom::DocCustomRelease->import();

            $self->{oDoc} =
                (new BackRestDoc::Custom::DocCustomRelease(${$self->{oManifest}->sourceGet('release')}{doc}, $self))->docGet();
        }
        else
        {
            $self->{oDoc} = ${$self->{oManifest}->sourceGet($self->{strRenderOutKey})}{doc};
        }

        $self->{oSource} = $self->{oManifest}->sourceGet($$oRenderOut{source});
    }

    if (defined($self->{strRenderOutKey}))
    {
        # Build the doc
        $self->build($self->{oDoc});

        # Get required sections
        foreach my $strPath (@{$self->{oManifest}->{stryRequire}})
        {
            if (substr($strPath, 0, 1) ne '/')
            {
                confess &log(ERROR, "path ${strPath} must begin with a /");
            }

            if (!defined($self->{oSection}->{$strPath}))
            {
                confess &log(ERROR, "required section '${strPath}' does not exist");
            }

            if (defined(${$self->{oSection}}{$strPath}))
            {
                $self->required($strPath);
            }
        }
    }

    if (defined($self->{oDoc}))
    {
        $self->{bToc} = !defined($self->{oDoc}->paramGet('toc', false)) || $self->{oDoc}->paramGet('toc') eq 'y' ? true : false;
        $self->{bTocNumber} =
            $self->{bToc} &&
            (!defined($self->{oDoc}->paramGet('toc-number', false)) || $self->{oDoc}->paramGet('toc-number') eq 'y') ? true : false;
    }

    # Return from function and log return values if any
    return logDebugReturn
    (
        $strOperation,
        {name => 'self', value => $self}
    );
}

####################################################################################################################################
# variableReplace
#
# Replace variables in the string.
####################################################################################################################################
sub variableReplace
{
    my $self = shift;

    return $self->{oManifest}->variableReplace(shift, $self->{strType});
}

####################################################################################################################################
# variableSet
#
# Set a variable to be replaced later.
####################################################################################################################################
sub variableSet
{
    my $self = shift;

    return $self->{oManifest}->variableSet(shift, shift);
}

####################################################################################################################################
# variableGet
#
# Get the current value of a variable.
####################################################################################################################################
sub variableGet
{
    my $self = shift;

    return $self->{oManifest}->variableGet(shift);
}

####################################################################################################################################
# build
#
# Build the section map and perform keyword matching.
####################################################################################################################################
sub build
{
    my $self = shift;
    my $oNode = shift;
    my $oParent = shift;
    my $strPath = shift;
    my $strPathPrefix = shift;

    # &log(INFO, "        node " . $oNode->nameGet());

    my $strName = $oNode->nameGet();

    if (defined($oParent))
    {
        if (!$self->{oManifest}->keywordMatch($oNode->paramGet('keyword', false)))
        {
            my $strDescription;

            if (defined($oNode->nodeGet('title', false)))
            {
                $strDescription = $self->processText($oNode->nodeGet('title')->textGet());
            }

            &log(DEBUG, "            filtered ${strName}" . (defined($strDescription) ? ": ${strDescription}" : ''));

            $oParent->nodeRemove($oNode);
        }
    }
    else
    {
            &log(DEBUG, '        build document');
            $self->{oSection} = {};
    }

    # Build section
    if ($strName eq 'section')
    {
        my $strSectionId = $oNode->paramGet('id');
        &log(DEBUG, "build section [${strSectionId}]");

        # Set path and parent-path for this section
        if (defined($strPath))
        {
            $oNode->paramSet('path-parent', $strPath);
        }

        $strPath .= '/' . $oNode->paramGet('id');

        &log(DEBUG, "            path ${strPath}");
        ${$self->{oSection}}{$strPath} = $oNode;
        $oNode->paramSet('path', $strPath);

        # If depend is not set then set it to the last section
        my $strDepend = $oNode->paramGet('depend', false);

        my $oContainerNode = defined($oParent) ? $oParent : $self->{oDoc};
        my $oLastChild;
        my $strDependPrev;

        foreach my $oChild ($oContainerNode->nodeList('section', false))
        {
            if ($oChild->paramGet('id') eq $oNode->paramGet('id'))
            {
                if (defined($oLastChild))
                {
                    $strDependPrev = $oLastChild->paramGet('id');
                }
                elsif (defined($oParent->paramGet('depend', false)))
                {
                    $strDependPrev = $oParent->paramGet('depend');
                }

                last;
            }

            $oLastChild = $oChild;
        }

        if (defined($strDepend))
        {
            if (defined($strDependPrev) && $strDepend eq $strDependPrev && !$oNode->paramTest('depend-default'))
            {
                &log(WARN,
                    "section '${strPath}' depend is set to '${strDepend}' which is the default, best to remove" .
                    " because it may become obsolete if a new section is added in between");
            }
        }
        else
        {
            $strDepend = $strDependPrev;
        }

        # If depend is defined make sure it exists
        if (defined($strDepend))
        {
            # If this is a relative depend then prepend the parent section
            if (index($strDepend, '/') != 0)
            {
                if (defined($oParent->paramGet('path', false)))
                {
                    $strDepend = $oParent->paramGet('path') . '/' . $strDepend;
                }
                else
                {
                    $strDepend = "/${strDepend}";
                }
            }

            if (!defined($self->{oSection}->{$strDepend}))
            {
                confess &log(ERROR, "section '${strSectionId}' depend '${strDepend}' is not valid");
            }
        }

        if (defined($strDepend))
        {
            $oNode->paramSet('depend', $strDepend);
        }

        if (defined($strDependPrev))
        {
            $oNode->paramSet('depend-default', $strDependPrev);
        }

        # If section content is being pulled from elsewhere go get the content
        if ($oNode->paramTest('source'))
        {
            my $oSource = ${$self->{oManifest}->sourceGet($oNode->paramGet('source'))}{doc};

            foreach my $oSection ($oSource->nodeList('section'))
            {
                push(@{${$oNode->{oDoc}}{children}}, $oSection->{oDoc});
            }

            # Set path prefix to modify all section paths further down
            $strPathPrefix = $strPath;
        }
    }
    # Build link
    elsif ($strName eq 'link')
    {
        &log(DEBUG, 'build link [' . $oNode->valueGet() . ']');

        # If the path prefix is set and this is a section
        if (defined($strPathPrefix) && $oNode->paramTest('section'))
        {
            my $strNewPath = $strPathPrefix . $oNode->paramGet('section');
            &log(DEBUG, "modify link section from '" . $oNode->paramGet('section') . "' to '${strNewPath}'");

            $oNode->paramSet('section', $strNewPath);
        }
    }
    # Store block defines
    elsif ($strName eq 'block-define')
    {
        my $strBlockId = $oNode->paramGet('id');

        if (defined($self->{oyBlockDefine}{$strBlockId}))
        {
            confess &log(ERROR, "block ${strBlockId} is already defined");
        }

        $self->{oyBlockDefine}{$strBlockId} = dclone($oNode->{oDoc}{children});
        $oParent->nodeRemove($oNode);
    }
    # Copy blocks
    elsif ($strName eq 'block')
    {
        my $strBlockId = $oNode->paramGet('id');

        if (!defined($self->{oyBlockDefine}{$strBlockId}))
        {
            confess &log(ERROR, "block ${strBlockId} is not defined");
        }

        my $strNodeJSON = $self->{oJSON}->encode($self->{oyBlockDefine}{$strBlockId});

        foreach my $oVariable ($oNode->nodeList('block-variable-replace'))
        {
            my $strVariableKey = $oVariable->paramGet('key');
            my $strVariableReplace = $oVariable->valueGet();

            $strNodeJSON =~ s/\{\[$strVariableKey\]\}/$strVariableReplace/g;
        }

        my ($iReplaceIdx, $iReplaceTotal) = $oParent->nodeReplace($oNode, $self->{oJSON}->decode($strNodeJSON));

        # Build any new children that were added
        my $iChildIdx = 0;

        foreach my $oChild ($oParent->nodeList(undef, false))
        {
            if ($iChildIdx >= $iReplaceIdx && $iChildIdx < ($iReplaceIdx + $iReplaceTotal))
            {
                $self->build($oChild, $oParent, $strPath, $strPathPrefix);
            }

            $iChildIdx++;
        }
    }

    # Iterate all text nodes
    if (defined($oNode->textGet(false)))
    {
        foreach my $oChild ($oNode->textGet()->nodeList(undef, false))
        {
            if (ref(\$oChild) ne "SCALAR")
            {
                $self->build($oChild, $oNode, $strPath, $strPathPrefix);
            }
        }
    }

    # Iterate all non-text nodes
    foreach my $oChild ($oNode->nodeList(undef, false))
    {
        if (ref(\$oChild) ne "SCALAR")
        {
            $self->build($oChild, $oNode, $strPath, $strPathPrefix);
        }
    }
}

####################################################################################################################################
# required
#
# Build a list of required sections
####################################################################################################################################
sub required
{
    my $self = shift;
    my $strPath = shift;
    my $bDepend = shift;

    # If node is not found that means the path is invalid
    my $oNode = ${$self->{oSection}}{$strPath};

    if (!defined($oNode))
    {
        confess &log(ERROR, "invalid path ${strPath}");
    }

    # Only add sections that are listed dependencies
    if (!defined($bDepend) || $bDepend)
    {
        # Match section and all child sections
        foreach my $strChildPath (sort(keys(%{$self->{oSection}})))
        {
            if ($strChildPath =~ /^$strPath$/ || $strChildPath =~ /^$strPath\/.*$/)
            {
                if (!defined(${$self->{oSectionRequired}}{$strChildPath}))
                {
                    my @stryChildPath = split('/', $strChildPath);

                    &log(INFO, ('    ' x (scalar(@stryChildPath) - 2)) . "        require section: ${strChildPath}");

                    ${$self->{oSectionRequired}}{$strChildPath} = true;
                }
            }
        }
    }

    # Get the path of the current section's parent
    my $strParentPath = $oNode->paramGet('path-parent', false);

    if ($oNode->paramTest('depend'))
    {
        foreach my $strDepend (split(',', $oNode->paramGet('depend')))
        {
            if ($strDepend !~ /^\//)
            {
                if (!defined($strParentPath))
                {
                    $strDepend = "/${strDepend}";
                }
                else
                {
                    $strDepend = "${strParentPath}/${strDepend}";
                }
            }

            $self->required($strDepend, true);
        }
    }
    elsif (defined($strParentPath))
    {
        $self->required($strParentPath, false);
    }
}

####################################################################################################################################
# isRequired
#
# Is it required to execute the section statements?
####################################################################################################################################
sub isRequired
{
    my $self = shift;
    my $oSection = shift;

    if (!defined($self->{oSectionRequired}))
    {
        return true;
    }

    my $strPath = $oSection->paramGet('path');

    defined(${$self->{oSectionRequired}}{$strPath}) ? true : false;
}

####################################################################################################################################
# processTag
####################################################################################################################################
sub processTag
{
    my $self = shift;

    # Assign function parameters, defaults, and log debug info
    my
    (
        $strOperation,
        $oTag
    ) =
        logDebugParam
        (
            __PACKAGE__ . '->processTag', \@_,
            {name => 'oTag', trace => true}
        );

    my $strBuffer = "";

    my $strType = $self->{strType};
    my $strTag = $oTag->nameGet();

    if (!defined($strTag))
    {
        require Data::Dumper;
        confess Dumper($oTag);
    }

    if ($strTag eq 'link')
    {
        my $strUrl = $oTag->paramGet('url', false);

        if (!defined($strUrl))
        {
            my $strPage = $self->variableReplace($oTag->paramGet('page', false));

            # If this is a page URL
            if (defined($strPage))
            {
                # If the page wasn't rendered then point at the website
                if (!defined($self->{oManifest}->renderOutGet($strType, $strPage, true)))
                {
                    $strUrl = '{[backrest-url-base]}/' . $oTag->paramGet('page') . '.html';
                }
                # Else point locally
                else
                {
                    if ($strType eq 'html' || $strType eq 'markdown')
                    {
                        $strUrl =
                            $oTag->paramGet('page', false) . '.' .
                            ($strType eq 'html' ? $strType : '.md');
                    }
                    else
                    {
                        confess &log(ERROR, "page links not supported for type ${strType}, value '" . $oTag->valueGet() . "'");
                    }
                }
            }
            else
            {
                my $strSection = $oTag->paramGet('section');
                my $oSection = ${$self->{oSection}}{$strSection};

                if (!defined($oSection))
                {
                    confess &log(ERROR, "section link '${strSection}' does not exist");
                }

                if (!defined($strSection))
                {
                    confess &log(ERROR, "link with value '" . $oTag->valueGet() . "' must defined url, page, or section");
                }

                if ($strType eq 'html')
                {
                    $strUrl = '#' . substr($strSection, 1);
                }
                elsif ($strType eq 'latex')
                {
                    $strUrl = $strSection;
                }
                else
                {
                    $strUrl = lc($self->processText($oSection->nodeGet('title')->textGet()));
                    $strUrl =~ s/[^\w\- ]//g;
                    $strUrl =~ s/ /-/g;
                    $strUrl = '#' . $strUrl;
                }
            }
        }

        if ($strType eq 'html')
        {
            $strBuffer = '<a href="' . $strUrl . '">' . $oTag->valueGet() . '</a>';
        }
        elsif ($strType eq 'markdown')
        {
            $strBuffer = '[' . $oTag->valueGet() . '](' . $strUrl . ')';
        }
        elsif ($strType eq 'latex')
        {
            if ($oTag->paramTest('url'))
            {
                $strBuffer = "\\href{$strUrl}{" . $oTag->valueGet() . "}";
            }
            else
            {
                $strBuffer = "\\hyperref[$strUrl]{" . $oTag->valueGet() . "}";
            }
        }
        else
        {
            confess "'link' tag not valid for type ${strType}";
        }
    }
    else
    {
        my $strStart = $$oRenderTag{$strType}{$strTag}[0];
        my $strStop = $$oRenderTag{$strType}{$strTag}[1];

        if (!defined($strStart) || !defined($strStop))
        {
            confess &log(ERROR, "invalid type ${strType} or tag ${strTag}");
        }

        $strBuffer .= $strStart;

        if ($strTag eq 'p' || $strTag eq 'title' || $strTag eq 'li' || $strTag eq 'code-block' || $strTag eq 'summary')
        {
            $strBuffer .= $self->processText($oTag);
        }
        elsif (defined($oTag->valueGet()))
        {
            $strBuffer .= $oTag->valueGet();
        }
        else
        {
            foreach my $oSubTag ($oTag->nodeList(undef, false))
            {
                $strBuffer .= $self->processTag($oSubTag);
            }
        }

        $strBuffer .= $strStop;
    }

    # Return from function and log return values if any
    return logDebugReturn
    (
        $strOperation,
        {name => 'strBuffer', value => $strBuffer, trace => true}
    );
}

####################################################################################################################################
# processText
####################################################################################################################################
sub processText
{
    my $self = shift;

    # Assign function parameters, defaults, and log debug info
    my
    (
        $strOperation,
        $oText
    ) =
        logDebugParam
        (
            __PACKAGE__ . '->processText', \@_,
            {name => 'oText', trace => true}
        );

    my $strType = $self->{strType};
    my $strBuffer = '';

    foreach my $oNode ($oText->nodeList(undef, false))
    {
        if (ref(\$oNode) eq "SCALAR")
        {
            if ($oNode =~ /\"/)
            {
                confess &log(ERROR, "unable to process quotes in string (use <quote> instead):\n${oNode}");
            }

            $strBuffer .= $oNode;
        }
        else
        {
            $strBuffer .= $self->processTag($oNode);
        }
    }
    #
    # if ($strType eq 'html')
    # {
    #         # $strBuffer =~ s/^\s+|\s+$//g;
    #
    #     $strBuffer =~ s/\n/\<br\/\>\n/g;
    # }

    # if ($strType eq 'markdown')
    # {
            # $strBuffer =~ s/^\s+|\s+$//g;

        $strBuffer =~ s/ +/ /g;
        $strBuffer =~ s/^ //smg;
    # }

    if ($strType eq 'latex')
    {
        $strBuffer =~ s/\&mdash\;/---/g;
        $strBuffer =~ s/\&lt\;/\</g;
        $strBuffer =~ s/\<\=/\$\\leq\$/g;
        $strBuffer =~ s/\>\=/\$\\geq\$/g;
        # $strBuffer =~ s/\_/\\_/g;
    }

    $strBuffer = $self->variableReplace($strBuffer);

    # Return from function and log return values if any
    return logDebugReturn
    (
        $strOperation,
        {name => 'strBuffer', value => $strBuffer, trace => true}
    );
}

1;
